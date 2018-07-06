#include <chrono>
#include <thread>
#include <Python.h>
#include <structmember.h>

#include "common.h"
#include "uv.h"
#include "python_server.h"
#include "entity.h"

int main(int argc, char *argv[])
{
	// Py_SetPythonHome(L"/home/mattos");
	// Initialise python
	// Py_VerboseFlag = 2;
	Py_FrozenFlag = 1;
	// Warn if tab and spaces are mixed in indentation.
	// Py_TabcheckFlag = 1;
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	Py_SetProgramName(L"myapp");
	Py_Initialize();
	if (!Py_IsInitialized())
	{
		fmt::print("Script::install(): Py_Initialize is failed!\n");
		return 1;
	}

	//把../src/myApp/script加入到 sys.path环境中
	PyRun_SimpleString(
		"import sys;"
		"import os;"
		"print('stdin: {0.encoding}:{0.errors}'.format(sys.stdin));"
		"print('stdout: {0.encoding}:{0.errors}'.format(sys.stdout));"
		"print('stderr: {0.encoding}:{0.errors}'.format(sys.stderr));"
		"sys.path.append(os.path.abspath(os.path.join('../src/myApp/script')));"
		"print(sys.path);"
		"from pathlib import Path;"
		"print('home: {}'.format(str(Path.home())));"
		"print(sys.version);"
		"sys.stdout.flush();");

	PyObject *mMain = PyImport_AddModule("__main__");

	//在python环境中添加Tutorial模块, 和程序相关的都存储在这里
	PyObject *newModule = PyImport_AddModule("Tutorial");
	bool ret = Entity::installScriptToModule(newModule, "Entity");
	if (!ret)
	{
		Py_Finalize();
		return 1;
	}
	PyObject *pyEntryScriptFileName = PyUnicode_FromString("game");
	PyObject *entryScript = PyImport_Import(pyEntryScriptFileName);
	if (entryScript == NULL)
	{
		PyErr_PrintEx(0);
		Py_Finalize();
		return 1;
	}
	PyObject *pyResult = PyObject_CallMethod(entryScript, "onMyAppRun", "O", PyBool_FromLong(1));
	if (pyResult != NULL)
	{
		Py_DECREF(pyResult);
	}
	else
	{
		PyErr_PrintEx(0);
		Py_Finalize();
		return 1;
	}
	uv_loop_t *loop = uv_default_loop();
	PythonServer pythonS;
	ret = pythonS.startup(loop);
	PythonConnection::installScriptToModule(mMain, "PythonConnection");
	if (!ret)
	{
		Py_Finalize();
		return 1;
	}
	uv_run(loop, UV_RUN_DEFAULT);
	Py_Finalize();
	return 0;
}
