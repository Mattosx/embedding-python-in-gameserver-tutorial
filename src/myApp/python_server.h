#ifndef _PYTHON_SERVER_HEADER
#define _PYTHON_SERVER_HEADER

#include "Python.h"

#include <deque>
#include <string>
#include <vector>

#include <uv.h>

using namespace std;

class PythonServer;

#ifndef _WIN32  // WIN32PORT
#else //ifndef _WIN32  // WIN32PORT
typedef unsigned short uint16_t;
#endif //ndef _WIN32  // WIN32PORT

class PythonConnection
{
public:
	PythonConnection( PythonServer* owner,
			uv_tcp_t *client,
			std::string welcomeString );
	virtual ~PythonConnection();

	void 						write( const char* str );
	void						writePrompt();

	bool						active() const	{ return active_; }

	static void onReadDataWrapper(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	void onReadData(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

private:
	bool						handleTelnetCommand();
	bool						handleVTCommand();
	void						handleLine();
	void						handleDel();
	void						handleChar();
	void						handleUp();
	void						handleDown();
	void						handleLeft();
	void						handleRight();

	uv_tcp_t*                   pClient_;
	std::deque<unsigned char>	readBuffer_;
	std::deque<std::string>		historyBuffer_;
	std::string					currentLine_;
	std::string					currentCommand_;
	PythonServer*				owner_;
	bool						telnetSubnegotiation_;
	int							historyPos_;
	unsigned int				charPos_;
	bool						active_;
	std::string					multiline_;
};

class PythonServer: public PyObject
{

public:
	PythonServer( std::string welcomeString = "Welcome to PythonServer." );
	virtual ~PythonServer();

	bool			startup( uv_loop_t *loop);
	void			shutdown();
	void			deleteConnection( PythonConnection* pConnection );
	
	static void onNewConnectionWrapper(uv_stream_t *server, int status);
	void onNewConnection(uv_stream_t *server, int status);

protected:
	void			printMessage( const std::string & msg );
private:

	std::vector<PythonConnection*> connections_;

	uv_tcp_t        server_;
	PyObject *		prevStderr_;
	PyObject *		prevStdout_;
	PyObject *		pSysModule_;
	std::string		welcomeString_;
};

#endif // _PYTHON_SERVER_HEADER
