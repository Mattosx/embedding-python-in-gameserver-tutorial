#ifndef _ENTITY_HEADER
#define _ENTITY_HEADER

#include <Python.h>
#include <structmember.h>

#include "common.h"

class Entity: public PyObject
{
    public:
        Entity();
        ~Entity();

        int setName(const char* newName);
        static bool installScriptToModule(PyObject* module, const char* name);

        uint64_t id;
        std::string name;

        static PyTypeObject _typeObject;
        static PyMemberDef _scriptMembers[];
        static PyGetSetDef _scriptSetGeters[];
        static PyMethodDef _scriptMethods[];

        static PyMethodDef _scriptExtraMethods;
};

#endif //_ENTITY_HEADER