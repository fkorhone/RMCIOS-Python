#ifndef PYTHON_H
#define PYTHOn_H

#include <wchar.h>

#define PyAPI_FUNC(RTYPE) __declspec(dllexport) RTYPE

#define Py_single_input 256
#define Py_file_input 257
#define Py_eval_input 258
#define Py_func_type_input 345

typedef enum { 
  PyGILState_LOCKED, 
  PyGILState_UNLOCKED
} PyGILState_STATE;

PyAPI_FUNC(void) (* Py_Initialize)(void);
PyAPI_FUNC(void) (* PyEval_InitThreads)(void);
PyAPI_FUNC(PyGILState_STATE) (* PyGILState_Ensure)(void);
PyAPI_FUNC(void) (* PyGILState_Release)(PyGILState_STATE);
PyAPI_FUNC(int) (* PyRun_SimpleString)(const char *s);

typedef struct _object PyObject;

#endif 
