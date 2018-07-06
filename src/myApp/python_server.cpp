#include "python_server.h"

#include <node.h>
#include <grammar.h>
#include <parsetok.h>
#include <errcode.h>
#include <ceval.h>
#include <functional>
#include <iostream>
#include <thread>
#include <structmember.h>

extern grammar _PyParser_Grammar;

#define TELNET_SERVER_PORT 9999
#define DEFAULT_BACKLOG 128

#define TELNET_ECHO 1
#define TELNET_LINEMODE 34
#define TELNET_SE 240
#define TELNET_SB 250
#define TELNET_WILL 251
#define TELNET_DO 252
#define TELNET_WONT 253
#define TELNET_DONT 254
#define TELNET_IAC 255

#define ERASE_EOL "\033[K"

#define KEY_CTRL_C 3
#define KEY_CTRL_D 4
#define KEY_BACKSPACE 8
#define KEY_DEL 127
#define KEY_ESC 27

#define MAX_HISTORY_LINES 50

typedef struct
{
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	buf->base = (char *)malloc(suggested_size);
	buf->len = suggested_size;
}

void free_write_req(uv_write_t *req)
{
	write_req_t *wr = (write_req_t *)req;
	free(wr->buf.base);
	free(wr);
}

void echo_write(uv_write_t *req, int status)
{
	if (status)
	{
		fprintf(stderr, "Write error %s\n", uv_strerror(status));
	}
	free_write_req(req);
}

static void PythonConnection_dealloc(PyObject *self)
{
	delete static_cast<PythonConnection *>(self);
}

static PyObject *PythonConnection_write(PyObject *self, PyObject *args)
{
	static_cast<PythonConnection *>(self)->writeFromPython(args);
}
PyObject *PythonConnection_flush(PyObject *self, PyObject *args)
{
	fmt::print("PythonConnection_flush\n");
	return Py_None;
}

PyMethodDef PythonConnection::_scriptMethods[] = {
	{"write", (PyCFunction)PythonConnection_write, METH_VARARGS, "write"},
	{"flush", (PyCFunction)PythonConnection_flush, METH_VARARGS, "flush"},
	{NULL} /* Sentinel */
};

PyMemberDef PythonConnection::_scriptMembers[] = {
	{"softspace", T_CHAR, offsetof(PythonConnection, softspace_), 0, "soft"},
	{NULL} /* Sentinel */
};

PyTypeObject PythonConnection::_typeObject = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0) "PythonConnection", /* tp_name */
	sizeof(PythonConnection),								   /* tp_basicsize */
	0,														   /* tp_itemsize */
	(destructor)PythonConnection_dealloc,					   /* tp_dealloc */
	0,														   /* tp_print */
	0,														   /* tp_getattr */
	0,														   /* tp_setattr */
	0,														   /* tp_reserved */
	0,														   /* tp_repr */
	0,														   /* tp_as_number */
	0,														   /* tp_as_sequence */
	0,														   /* tp_as_mapping */
	0,														   /* tp_hash  */
	0,														   /* tp_call */
	0,														   /* tp_str */
	PyObject_GenericGetAttr,								   /* tp_getattro */
	0,														   /* tp_setattro */
	0,														   /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
		Py_TPFLAGS_BASETYPE,		  /* tp_flags */
	"PythonConnection objects",		  /* tp_doc */
	0,								  /* tp_traverse */
	0,								  /* tp_clear */
	0,								  /* tp_richcompare */
	0,								  /* tp_weaklistoffset */
	0,								  /* tp_iter */
	0,								  /* tp_iternext */
	PythonConnection::_scriptMethods, /* tp_methods */
	PythonConnection::_scriptMembers, /* tp_members */
	0,								  /* tp_getset */
	0,								  /* tp_base */
	0,								  /* tp_dict */
	0,								  /* tp_descr_get */
	0,								  /* tp_descr_set */
	0,								  /* tp_dictoffset */
	0,								  /* tp_init */
	0,								  /* tp_alloc */
	0,								  /* tp_new */
	PyObject_GC_Del,				  /* tp_free */
};

PythonConnection::PythonConnection(PythonServer *owner, uv_tcp_t *client) : prevStderr_(NULL),
																			prevStdout_(NULL),
																			pClient_(client),
																			owner_(owner),
																			telnetSubnegotiation_(false),
																			historyPos_(-1),
																			charPos_(0),
																			active_(false),
																			multiline_(),
																			softspace_(0)
{
	PyObject_INIT(static_cast<PyObject *>(this), &_typeObject);

	pClient_->data = this;

	unsigned char options[] =
		{
			TELNET_IAC, TELNET_WILL, TELNET_ECHO,
			TELNET_IAC, TELNET_WONT, TELNET_LINEMODE, 0};

	this->write((char *)options);
	this->write("welcome to python console");
	this->write("\r\n");
	this->writePrompt();

	uv_read_start((uv_stream_t *)pClient_, alloc_buffer, PythonConnection::onReadDataWrapper);
}

PythonConnection::~PythonConnection()
{
	uv_read_stop((uv_stream_t *)pClient_);
	uv_close((uv_handle_t *)pClient_, NULL);
	free(pClient_);
}

void PythonConnection::writeFromPython(PyObject *args)
{
	PyObject *obj = NULL;
	if (!PyArg_ParseTuple(args, "O", &obj))
	{
		return;
	}

	Py_ssize_t size = 0;
	const char *ret = PyUnicode_AsUTF8AndSize(obj, &size);
	if (!ret)
	{
		return;
	}
	std::string msg(ret, size);

	PyObject *pResult = PyObject_CallMethod(prevStdout_, "write", "s#", msg.c_str(), msg.length());

	if (pResult)
	{
		Py_DECREF(pResult);
	}
	else
	{
		PyErr_Clear();
	}
	std::string outputdMsg;
	outputdMsg.reserve(msg.size());
	std::string::const_iterator iter;
	for (iter = msg.begin(); iter != msg.end(); iter++)
	{
		if (*iter == '\n')
			outputdMsg += "\r\n";
		else
			outputdMsg += *iter;
	}

	if (active())
	{
		write(outputdMsg.c_str());
	}
}
bool PythonConnection::installScriptToModule(PyObject *module, const char *name)
{
	if (PyType_Ready(&_typeObject) < 0)
	{
		PyErr_Print();
		return false;
	}
	Py_INCREF(&_typeObject);
	return true;
}
void PythonConnection::onReadDataWrapper(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	PythonConnection *instance = static_cast<PythonConnection *>(stream->data);
	instance->onReadData(stream, nread, buf);
}

void PythonConnection::onReadData(uv_stream_t *stream, ssize_t bytesRead, const uv_buf_t *buf_)
{
	char *buf = buf_->base;

	if (bytesRead == -1)
	{
		printf("PythonConnection: Read error\n");
		owner_->deleteConnection(this);
		return;
	}

	if (bytesRead == 0)
	{
		owner_->deleteConnection(this);
		return;
	}

	for (ssize_t i = 0; i < bytesRead; i++)
	{
		readBuffer_.push_back(buf[i]);
	}

	while (!readBuffer_.empty())
	{
		int c = (unsigned char)readBuffer_[0];

		// Handle (and ignore) telnet protocol commands.

		if (c == TELNET_IAC)
		{
			if (!this->handleTelnetCommand())
				return;
			continue;
		}

		if (c == KEY_ESC)
		{
			if (!this->handleVTCommand())
				return;
			continue;
		}

		// If we're in telnet subnegotiation mode, ignore normal chars.

		if (telnetSubnegotiation_)
		{
			readBuffer_.pop_front();
			continue;
		}

		switch (c)
		{
		case '\r':
			this->handleLine();
			break;

		case KEY_BACKSPACE:
		case KEY_DEL:
			this->handleDel();
			break;

		case KEY_CTRL_C:
		case KEY_CTRL_D:
			owner_->deleteConnection(this);
			return;

		case '\0':
		case '\n':
			// Ignore these
			readBuffer_.pop_front();
			break;

		default:
			this->handleChar();
			break;
		}
	}

	return;
}

/**
 * 	This method handles telnet protocol commands. Well actually it handles
 * 	a subset of telnet protocol commands, enough to get Linux and Windows
 *	telnet working in character mode.
 */
bool PythonConnection::handleTelnetCommand()
{
	// TODO: Need to check that there is a second byte on readBuffer_.
	unsigned int cmd = (unsigned char)readBuffer_[1];
	unsigned int bytesNeeded = 2;
	char str[256];

	switch (cmd)
	{
	case TELNET_WILL:
	case TELNET_WONT:
	case TELNET_DO:
	case TELNET_DONT:
		bytesNeeded = 3;
		break;

	case TELNET_SE:
		telnetSubnegotiation_ = false;
		break;

	case TELNET_SB:
		telnetSubnegotiation_ = true;
		break;

	case TELNET_IAC:
		// A literal 0xff. We don't care!
		break;

	default:
		sprintf(str, "Telnet command %d unsupported.\r\n", cmd);
		this->write(str);
		break;
	}

	if (readBuffer_.size() < bytesNeeded)
		return false;

	while (bytesNeeded)
	{
		bytesNeeded--;
		readBuffer_.pop_front();
	}

	return true;
}

bool PythonConnection::handleVTCommand()
{
	// Need 3 chars before we are ready.
	if (readBuffer_.size() < 3)
		return false;

	// Eat the ESC.
	readBuffer_.pop_front();

	if (readBuffer_.front() != '[' && readBuffer_.front() != 'O')
		return true;

	// Eat the [
	readBuffer_.pop_front();

	switch (readBuffer_.front())
	{
	case 'A':
		this->handleUp();
		break;

	case 'B':
		this->handleDown();
		break;

	case 'C':
		this->handleRight();
		break;

	case 'D':
		this->handleLeft();
		break;

	default:
		return true;
	}

	readBuffer_.pop_front();
	return true;
}

/**
 * 	This method handles a single character. It appends or inserts it
 * 	into the buffer at the current position.
 */
void PythonConnection::handleChar()
{
	// @todo: Optimise redraw
	currentLine_.insert(charPos_, 1, (char)readBuffer_.front());
	int len = currentLine_.length() - charPos_;
	this->write(currentLine_.substr(charPos_, len).c_str());

	for (int i = 0; i < len - 1; i++)
		this->write("\b");

	charPos_++;
	readBuffer_.pop_front();
}

void PythonConnection::hookStdOutErr()
{
	fmt::print("hookStdOutErr\n");
	PyObject *pSysModule_ = PyImport_ImportModule("sys");
	if (!pSysModule_)
	{
		return;
	}

	prevStderr_ = PyObject_GetAttrString(pSysModule_, "stderr");
	prevStdout_ = PyObject_GetAttrString(pSysModule_, "stdout");

	PyObject_SetAttrString(pSysModule_, "stderr", (PyObject *)this);
	PyObject_SetAttrString(pSysModule_, "stdout", (PyObject *)this);
	Py_DECREF(pSysModule_);
	return;
}

void PythonConnection::unhookStdOutErr()
{
	fmt::print("unhookStdOutErr\n");
	PyObject *pSysModule_ = PyImport_ImportModule("sys");
	if (prevStderr_)
	{
		PyObject_SetAttrString(pSysModule_, "stderr", prevStderr_);
		Py_DECREF(prevStderr_);
		prevStderr_ = NULL;
	}

	if (prevStdout_)
	{
		PyObject_SetAttrString(pSysModule_, "stdout", prevStdout_);
		Py_DECREF(prevStdout_);
		prevStdout_ = NULL;
	}
	Py_DECREF(pSysModule_);
}

#if 0
/**
 * 	This method returns true if the command would fail because of an EOF
 * 	error. Could use this to implement multiline commands.. but later.
 */
static bool CheckEOF(char *str)
{
	node *n;
	perrdetail err;
	n = PyParser_ParseString(str, &_PyParser_Grammar, Py_single_input, &err);

	if (n == NULL && err.error == E_EOF )
	{
		printf("EOF\n");
		return true;
	}

	printf("OK\n");
	PyNode_Free(n);
	return false;
}
#endif

/**
 * 	This is a variant on PyRun_SimpleString. It does basically the
 *	same thing, but uses Py_single_input, so the Python compiler
 * 	will mark the code as being interactive, and print the result
 *	if it is not None.
 *
 *	@param command		Line of Python to execute.
 */
static int MyRun_SimpleString(char *command)
{
	// Ignore lines that only contain comments
	{
		char *pCurr = command;
		while (*pCurr != '\0' && *pCurr != ' ' && *pCurr != '\t')
		{
			if (*pCurr == '#')
				return 0;
			++pCurr;
		}
	}

	PyObject *m, *d, *v;
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);
	v = PyRun_String(command, Py_single_input, d, d);

	if (v == NULL)
	{
		PyErr_Print();
		return -1;
	}
	Py_DECREF(v);
	return 0;
}

/**
 * 	This method handles an end of line. It executes the current command,
 *	and adds it to the history buffer.
 */
void PythonConnection::handleLine()
{
	readBuffer_.pop_front();
	this->write("\r\n");

	if (currentLine_.empty())
	{
		currentLine_ = multiline_;
		multiline_ = "";
	}
	else
	{
		historyBuffer_.push_back(currentLine_);

		if (historyBuffer_.size() > MAX_HISTORY_LINES)
		{
			historyBuffer_.pop_front();
		}

		if (!multiline_.empty())
		{
			multiline_ += "\n" + currentLine_;
			currentLine_ = "";
		}
	}

	if (!currentLine_.empty())
	{
		currentLine_ += "\n";

		if (currentLine_[currentLine_.length() - 2] == ':')
		{
			multiline_ += currentLine_;
		}
		else
		{
			active_ = true;
			hookStdOutErr();
			MyRun_SimpleString((char *)currentLine_.c_str());
			unhookStdOutErr();
			// PyObject * pRes = PyRun_String( (char *)currentLine_.c_str(), true );
			active_ = false;
		}
	}

	currentLine_ = "";
	historyPos_ = -1;
	charPos_ = 0;

	this->writePrompt();
}

/**
 *	This method handles a del character.
 */
void PythonConnection::handleDel()
{

	if (charPos_ > 0)
	{
		// @todo: Optimise redraw
		currentLine_.erase(charPos_ - 1, 1);
		this->write("\b" ERASE_EOL);
		charPos_--;
		int len = currentLine_.length() - charPos_;
		this->write(currentLine_.substr(charPos_, len).c_str());

		for (int i = 0; i < len; i++)
			this->write("\b");
	}

	readBuffer_.pop_front();
}

/**
 * 	This method handles a key up event.
 */
void PythonConnection::handleUp()
{
	if (historyPos_ < (int)historyBuffer_.size() - 1)
	{
		historyPos_++;
		currentLine_ = historyBuffer_[historyBuffer_.size() -
									  historyPos_ - 1];

		// @todo: Optimise redraw
		this->write("\r" ERASE_EOL);
		this->writePrompt();
		this->write(currentLine_.c_str());
		charPos_ = currentLine_.length();
	}
}

/**
 * 	This method handles a key down event.
 */
void PythonConnection::handleDown()
{
	if (historyPos_ >= 0)
	{
		historyPos_--;

		if (historyPos_ == -1)
			currentLine_ = "";
		else
			currentLine_ = historyBuffer_[historyBuffer_.size() -
										  historyPos_ - 1];

		// @todo: Optimise redraw
		this->write("\r" ERASE_EOL);
		this->writePrompt();
		this->write(currentLine_.c_str());
		charPos_ = currentLine_.length();
	}
}

/**
 * 	This method handles a key left event.
 */
void PythonConnection::handleLeft()
{
	if (charPos_ > 0)
	{
		charPos_--;
		this->write("\033[D");
	}
}

/**
 * 	This method handles a key left event.
 */
void PythonConnection::handleRight()
{
	if (charPos_ < currentLine_.length())
	{
		charPos_++;
		this->write("\033[C");
	}
}

void PythonConnection::write(const char *str)
{
	write_req_t *req = (write_req_t *)malloc(sizeof(write_req_t));
	ssize_t len = strlen(str);
	if (len == 0)
	{
		// printf("write str error\n");
		return;
	}
	void *str1 = malloc(len);
	memcpy(str1, str, len);
	req->buf = uv_buf_init((char *)str1, len);
	uv_write((uv_write_t *)req, (uv_stream_t *)pClient_, &req->buf, 1, echo_write);
}

/**
 * 	This method prints a prompt to the socket.
 */
void PythonConnection::writePrompt()
{
	return this->write(multiline_.empty() ? ">>> " : "... ");
}

PythonServer::PythonServer()
{
}

PythonServer::~PythonServer()
{
	this->shutdown();
}

/*~ class PythonServer
 *	This class provides access to the Python interpreter via a TCP connection.
 *	It starts listening on a given port, and creates a new PythonConnection
 *	for every connection it receives.
 */

/**
 *	This method starts up the Python server, and begins listening on the
 *	given port. It redirects Python stdout and stderr, so that they can be
 *	sent to all Python connections as well as stdout.

 */
bool PythonServer::startup(uv_loop_t *loop)
{
	struct sockaddr_in addr;
	uv_tcp_init(loop, &server_);
	uv_ip4_addr("127.0.0.1", TELNET_SERVER_PORT, &addr);

	server_.data = this;
	uv_tcp_bind(&server_, (const struct sockaddr *)&addr, 0);

	int r = uv_listen((uv_stream_t *)&server_, DEFAULT_BACKLOG, PythonServer::onNewConnectionWrapper);
	if (r)
	{
		fprintf(stderr, "Listen error %s\n", uv_strerror(r));
		return false;
	}

	printf("Python server is running\n");

	return true;
}

void PythonServer::onNewConnectionWrapper(uv_stream_t *server, int status)
{
	PythonServer *instance = static_cast<PythonServer *>(server->data);
	instance->onNewConnection(server, status);
}

void PythonServer::onNewConnection(uv_stream_t *server, int status)
{
	if (status < 0)
	{
		fprintf(stderr, "New connection error %s\n", uv_strerror(status));
		return;
	}

	uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(server->loop, client);
	if (uv_accept(server, (uv_stream_t *)client) == 0)
	{
		connections_.push_back(new PythonConnection(this, client));
	}
	else
	{
		uv_close((uv_handle_t *)client, NULL);
	}
}

/**
 * 	This method shuts down the Python server.
 * 	It closes the listener port, disconnects all connections,
 * 	and restores Python stderr and stdout.
 */
void PythonServer::shutdown()
{
	std::vector<PythonConnection *>::iterator it;

	// Disconnect all connections, and clear our connection list.

	for (it = connections_.begin(); it != connections_.end(); it++)
	{
		delete *it;
	}

	connections_.clear();

	// Shutdown the listener socket if it is open.

	// If stderr and stdout were redirected, restore them.
}

/**
 * 	This method is called by Python whenever there is new data for
 * 	stdout or stderror. We redirect it to all the connections, and
 * 	then print it out as normal. CRs are subsituted for CR/LF pairs
 * 	to facilitate printing on Windows.
 *
 * 	@param args		A Python tuple containing a single string argument
 */
// void PythonServer::printMessage( const std::string & msg )
// {
// 	PyObject * pResult =
// 		PyObject_CallMethod( prevStdout_, "write", "s#", msg.c_str(), msg.length() );
// 	if (pResult)
// 	{
// 		Py_DECREF( pResult );
// 	}
// 	else
// 	{
// 		PyErr_Clear();
// 	}

// 	std::string cookedMsg;
// 	cookedMsg.reserve( msg.size() );
// 	std::string::const_iterator iter;
// 	for (iter = msg.begin(); iter != msg.end(); iter++)
// 	{
// 		if (*iter == '\n')
// 			cookedMsg += "\r\n";
// 		else
// 			cookedMsg += *iter;
// 	}

// 	std::vector<PythonConnection *>::iterator it;

// 	for(it = connections_.begin(); it != connections_.end(); it++)
// 	{
// 		if ((*it)->active())
// 		{
// 			(*it)->write( cookedMsg.c_str() );
// 		}
// 	}
// }

/**
 * 	This method deletes a connection from the python server.
 *
 *	@param pConnection	The connection to be deleted.
 */
void PythonServer::deleteConnection(PythonConnection *pConnection)
{
	std::vector<PythonConnection *>::iterator it;

	for (it = connections_.begin(); it != connections_.end(); it++)
	{
		if (*it == pConnection)
		{
			delete *it;
			connections_.erase(it);
			return;
		}
	}

	printf("PythonServer::deleteConnection: %p not found",
		   pConnection);
}
