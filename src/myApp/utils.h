#ifndef _UTILS_HEADER
#define _UTILS_HEADER

#include "common.h"
#include <assert.h>
#include <Python.h>

//genUUID64 生成的uint64 可以在分布式节点中直接使用
inline uint64_t genUUID64()
{
	static uint64_t tv = (uint64_t)(time(NULL));
	uint64_t now = (uint64_t)(time(NULL));

	static uint16_t lastNum = 0;

	if(now != tv)
	{
		tv = now;
		lastNum = 0;
	}
	// 时间戳32位，随机数16位，16位迭代数（最大为65535-1）
	static uint32_t rnd = 0;
	if(rnd == 0)
	{
		srand(getSystemTime());
		rnd = (uint32_t)(rand() << 16);
	}
	
	assert(lastNum < 65535 && "genUUID64(): overflow!");
	
	return (tv << 32) | rnd | lastNum++;
}

static int __readonly_descr(PyObject* self, PyObject* value, void* closure)
{
    PyErr_Format(PyExc_TypeError, "Sorry, this attribute %s is read-only", (self != NULL ? self->ob_type->tp_name : ""));
    PyErr_PrintEx(0);
    return 0;
}

#endif //_UTILS_HEADER