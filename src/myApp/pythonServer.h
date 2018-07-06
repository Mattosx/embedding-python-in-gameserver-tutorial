#ifndef _PYTHON_SERVER_HEADER
#define _PYTHON_SERVER_HEADER

#include "Python.h"

#include <deque>
#include <string>
#include <vector>

#include <uv.h>
#include "common.h"

using namespace std;

class PythonServer;

#ifndef _WIN32 // WIN32PORT
#else					 //ifndef _WIN32  // WIN32PORT
typedef unsigned short uint16_t;
#endif				 //ndef _WIN32  // WIN32PORT

class PythonConnection : public PyObject
{
public:
	PythonConnection(PythonServer *owner,
									 uv_tcp_t *client);
	virtual ~PythonConnection();

	void write(const char *str);
	void writePrompt();

	bool active() const { return active_; }

	static void onReadDataWrapper(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
	void onReadData(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
	void hookStdOutErr();
	void unhookStdOutErr();
	void writeFromPython(PyObject *args);
	static bool installScriptToModule(PyObject *module, const char *name);

private:
	bool handleTelnetCommand();
	bool handleVTCommand();
	void handleLine();
	void handleDel();
	void handleChar();
	void handleUp();
	void handleDown();
	void handleLeft();
	void handleRight();

	uv_tcp_t *pClient_;
	std::deque<unsigned char> readBuffer_;
	std::deque<std::string> historyBuffer_;
	std::string currentLine_;
	std::string currentCommand_;
	PythonServer *owner_;
	bool telnetSubnegotiation_;
	int historyPos_;
	unsigned int charPos_;
	bool active_;
	std::string multiline_;
	bool softspace_;

	PyObject *prevStderr_;
	PyObject *prevStdout_;

	static PyTypeObject _typeObject;
	static PyMethodDef _scriptMethods[];
	static PyMemberDef _scriptMembers[];
};

class PythonServer
{

public:
	PythonServer();
	virtual ~PythonServer();

	bool startup(uv_loop_t *loop, std::string ipaddress, int port);
	void shutdown();
	void deleteConnection(PythonConnection *pConnection);

	static void onNewConnectionWrapper(uv_stream_t *server, int status);
	void onNewConnection(uv_stream_t *server, int status);

protected:
	// void printMessage( const std::string & msg );
private:
	std::vector<PythonConnection *> connections_;

	uv_tcp_t server_;
};

#endif // _PYTHON_SERVER_HEADER
