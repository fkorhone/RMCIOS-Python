#ifndef PYTHON_H
#define PYTHOn_H

#define PyAPI_FUNC(RTYPE) __declspec(dllexport) RTYPE

typedef enum { 
  PyGILState_LOCKED, 
  PyGILState_UNLOCKED
} PyGILState_STATE;

PyAPI_FUNC(void) Py_Initialize(void);
PyAPI_FUNC(void) PyEval_InitThreads(void);
PyAPI_FUNC(PyGILState_STATE) PyGILState_Ensure(void);
PyAPI_FUNC(void) PyGILState_Release(PyGILState_STATE);
PyAPI_FUNC(int) PyRun_SimpleString(const char *s);

#endif 
