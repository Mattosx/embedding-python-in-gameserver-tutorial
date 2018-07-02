#include "entity.h"
#include "utils.h"
#include <unicodeobject.h>

static PyObject* _createEntity(PyObject* self, PyObject* args)
{
    PyObject* createType;
    if(!PyArg_ParseTuple(args, "O", &createType)){
        PyErr_Print();
        return Py_None;
    }
    if(!PyType_Check(createType)){
        PyErr_Print();
        return Py_None;
    }
	PyObject *pObject = PyType_GenericAlloc((PyTypeObject*)createType, 0);
	if (pObject == NULL)
	{
		PyErr_Print();
        return Py_None;
	}

    Entity* e = new(pObject) Entity();
	return pObject;
}	

static void Entity_dealloc(PyObject* self)	
{
    static_cast<Entity*>(self)->~Entity();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Entity_getName(PyObject *self, void *)
{   
    return PyUnicode_FromString(static_cast<Entity*>(self)->name.c_str());
}

static int Entiti_setName(PyObject *self, PyObject *value, void *closure){

    if (!PyUnicode_Check(value)) {
        return 0;
    }
    PyObject * temp_bytes = PyUnicode_AsEncodedString(value, "UTF-8", "strict"); // Owned reference
    if (temp_bytes != NULL) {
        char* my_result = PyBytes_AS_STRING(temp_bytes); // Borrowed pointer
        return static_cast<Entity*>(self)->setName(my_result);
        Py_DECREF(temp_bytes);
    } else {
        return 0;
    }
}

static PyObject* Entity_getId(PyObject* self, void*)
{
    return PyLong_FromUnsignedLongLong(static_cast<Entity*>(self)->id)
}
static PyObject * Entity_sendMessage(Entity* self, PyObject *args, PyObject *kwds)
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

PyMethodDef Entity::_scriptMethods[] = {
	{ "sendMessage", (PyCFunction)Entity_sendMessage, METH_VARARGS, "sendMessage"},
	{ NULL }  /* Sentinel */
};

PyMemberDef Entity::_scriptMembers[] = {
	{ "id", T_INT, offsetof(Entity, id), 0, "entity id" },
	{ NULL }  /* Sentinel */
};

PyGetSetDef Entity::_scriptSetGeters[] = {
    {"name", (getter)Entity_getName, (setter)Entiti_setName, 0, 0},
    {"id", (getter)Entity_getName, (setter)__py_readonly_descr, 0, 0},
    { NULL }
};

PyMethodDef Entity::_scriptExtraMethods = {"createEntity", (PyCFunction) _createEntity, METH_VARARGS, NULL};

PyTypeObject Entity::_typeObject = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"Entity",             /* tp_name */
	sizeof(Entity),             /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Entity_dealloc, /* tp_dealloc */
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
	Py_TPFLAGS_BASETYPE,        /* tp_flags */
	"Entity objects",           /* tp_doc */
	0,   /* tp_traverse */
	0,           /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Entity::_scriptMethods,             /* tp_methods */
	Entity::_scriptMembers,             /* tp_members */
	Entity::_scriptSetGeters,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	0,      /* tp_init */
	0,                         /* tp_alloc */
	0,                 /* tp_new */
};

Entity::Entity()
:id(genUUID64()),
name("")
{

}

Entity::~Entity(){

}

int Entity::setName(const char* newName){
    name = newName;
    return 1;
}
bool Entity::installScriptToModule(PyObject* module, const char* name){
    if (PyType_Ready(&_typeObject) < 0)
    {
			PyErr_Print();
			return false;
    }
    Py_INCREF(&_typeObject);
    if(PyModule_AddObject(module, name, (PyObject *)&_typeObject) < 0)
    {
        PyErr_Print();
        return false;

    }

	if(PyModule_AddObject(module, "createEntity", PyCFunction_New(&_scriptExtraMethods, 0)) != 0)
	{   
        PyErr_Print();
		return false;
	}

    return true;
}