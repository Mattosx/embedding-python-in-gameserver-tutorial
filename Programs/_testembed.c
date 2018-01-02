#include <Python.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <xstring>

#include <chrono>
#include <thread>

#include <Python.h>
#include "structmember.h"

typedef struct {
	PyObject_HEAD
		PyObject *first;
	PyObject *last;
	int number;
} Noddy;

static int
Noddy_traverse(Noddy *self, visitproc visit, void *arg)
{
	int vret;

	if (self->first) {
		vret = visit(self->first, arg);
		if (vret != 0)
			return vret;
	}
	if (self->last) {
		vret = visit(self->last, arg);
		if (vret != 0)
			return vret;
	}

	return 0;
}

static int
Noddy_clear(Noddy *self)
{
	PyObject *tmp;

	tmp = self->first;
	self->first = NULL;
	Py_XDECREF(tmp);

	tmp = self->last;
	self->last = NULL;
	Py_XDECREF(tmp);

	return 0;
}

static void
Noddy_dealloc(Noddy* self)
{
	PyObject_GC_UnTrack(self);
	Noddy_clear(self);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Noddy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Noddy *self;

	self = (Noddy *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->first = PyUnicode_FromString("");
		if (self->first == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->last = PyUnicode_FromString("");
		if (self->last == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->number = 0;
	}

	return (PyObject *)self;
}

static int
Noddy_init(Noddy *self, PyObject *args, PyObject *kwds)
{
	PyObject *first = NULL, *last = NULL, *tmp;

	static char *kwlist[] = { "first", "last", "number", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist,
		&first, &last,
		&self->number))
		return -1;

	if (first) {
		tmp = self->first;
		Py_INCREF(first);
		self->first = first;
		Py_XDECREF(tmp);
	}

	if (last) {
		tmp = self->last;
		Py_INCREF(last);
		self->last = last;
		Py_XDECREF(tmp);
	}

	return 0;
}


static PyMemberDef Noddy_members[] = {
	{ "first", T_OBJECT_EX, offsetof(Noddy, first), 0,
	"first name" },
	{ "last", T_OBJECT_EX, offsetof(Noddy, last), 0,
	"last name" },
	{ "number", T_INT, offsetof(Noddy, number), 0,
	"noddy number" },
	{ NULL }  /* Sentinel */
};

static PyObject *
Noddy_name(Noddy* self)
{
	if (self->first == NULL) {
		PyErr_SetString(PyExc_AttributeError, "first");
		return NULL;
	}

	if (self->last == NULL) {
		PyErr_SetString(PyExc_AttributeError, "last");
		return NULL;
	}

	return PyUnicode_FromFormat("%S %S", self->first, self->last);
}

static PyObject *
Noddy_sendMessage(Noddy* self, PyObject *args, PyObject *kwds)
{
	PyObject *methodName = NULL, *methodArgs = NULL;

	static char *kwlist[] = { "methodPort", "methodName", "methodArgs", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", kwlist, &methodName, &methodArgs))
		return NULL;

	if (methodName) {
		Py_INCREF(methodName);
	}

	if (methodArgs) {
		Py_INCREF(methodArgs);
	}

	return NULL;
}


static PyMethodDef Noddy_methods[] = {
	{ "name", (PyCFunction)Noddy_name, METH_NOARGS,
	"Return the name, combining the first and last name"
	},
	{ NULL }  /* Sentinel */
};

static PyTypeObject NoddyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"noddy.Noddy",             /* tp_name */
	sizeof(Noddy),             /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Noddy_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE |
	Py_TPFLAGS_HAVE_GC,    /* tp_flags */
	"Noddy objects",           /* tp_doc */
	(traverseproc)Noddy_traverse,   /* tp_traverse */
	(inquiry)Noddy_clear,           /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Noddy_methods,             /* tp_methods */
	Noddy_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Noddy_init,      /* tp_init */
	0,                         /* tp_alloc */
	Noddy_new,                 /* tp_new */
};

void enterLoop(PyObject* entryScript) {
	for (;;)
	{
		PyObject* pyResult = PyObject_CallMethod(entryScript, "onTimerUpdate", "l", time(NULL));
		std::this_thread::sleep_for(std::chrono::seconds(5));
		if (pyResult != NULL)
		{
			Py_DECREF(pyResult);
		}
		else {
			PyErr_PrintEx(0);
			break;
		}
	}

}

/* Different embedding tests */
int main(int argc, char *argv[])
{
	// Initialise python
	// Py_VerboseFlag = 2;
	Py_FrozenFlag = 1;

	// Warn if tab and spaces are mixed in indentation.
	// Py_TabcheckFlag = 1;
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	Py_SetProgramName(L"./_testembed");
	Py_Initialize();
	if (!Py_IsInitialized())
	{
		std::cout << "Script::install(): Py_Initialize is failed!\n";
		return 1;
	}

	PyObject *mMain = PyImport_AddModule("__main__");

	PyObject* newModule = PyImport_AddModule("noddy4");

	if (PyType_Ready(&NoddyType) < 0)
		return 1;

	Py_INCREF(&NoddyType);

	PyModule_AddObject(newModule, "Noddy", (PyObject *)&NoddyType);

	PyRun_SimpleString(
		"import sys;"
		"print('stdin: {0.encoding}:{0.errors}'.format(sys.stdin));"
		"print('stdout: {0.encoding}:{0.errors}'.format(sys.stdout));"
		"print('stderr: {0.encoding}:{0.errors}'.format(sys.stderr));"
		"sys.stdout.flush()\n"
		"print(sys.path)\n"
	);

	PyObject *pyEntryScriptFileName = PyUnicode_FromString("game");

	PyObject* entryScript = PyImport_Import(pyEntryScriptFileName);

	if (entryScript == NULL) {
		PyErr_PrintEx(0);
		return 1;
	}

	PyObject* pyResult = PyObject_CallMethod(entryScript, "onMyAppRun", "O", PyBool_FromLong(1));
	if (pyResult != NULL)
		Py_DECREF(pyResult);
	else
		PyErr_PrintEx(0);

	enterLoop(entryScript);
	std::this_thread::sleep_for(std::chrono::seconds(3));
	Py_Finalize();
	return 0;
}
