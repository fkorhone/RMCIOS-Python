/* 
RMCIOS - Reactive Multipurpose Control Input Output System
Copyright (c) 2018 Frans Korhonen

RMCIOS was originally developed at Institute for Atmospheric 
and Earth System Research / Physics, Faculty of Science, 
University of Helsinki, Finland

This file is part of RMCIOS. This notice was encoded using utf-8.

RMCIOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RMCIOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public Licenses
along with RMCIOS.  If not, see <http://www.gnu.org/licenses/>.
*/

#define PY_SSIZE_T_CLEAN
#include "RMCIOS-functions.h"
#include <winsock.h> // Needed for TIMEVAL structure in windows
#include <Python.h>

PyObject *param_to_python(const struct context_rmcios *context, 
                          enum type_rmcios paramtype, 
                          const union param_rmcios param, int index)
{
    PyObject *pValue;

    switch(paramtype)
    {
       case float_rmcios:
          {
             float fvalue;
             fvalue = param_to_float(context, paramtype, param, index);
             pValue = PyFloat_FromDouble(fvalue);
          }
          break;

        case int_rmcios:
          {
             int ivalue;
             ivalue = param_to_int(context, paramtype, param, index);
             pValue = PyLong_FromLong(ivalue);
          }
          break;

        case buffer_rmcios:
          {
             int len = param_string_alloc_size(context, paramtype, param, index);
             {
                const char *s;
                char buffer[len];
                s = param_to_string(context, paramtype, param, index, len, buffer);
                pValue = PyBytes_FromString(s);
             }
          }
          break;

        case binary_rmcios:
          {
             int len= param_binary_length(context, paramtype, param, index);
             {
                char buffer[len];
                param_to_binary(context, paramtype, param, index, len, buffer);
                pValue = PyBytes_FromStringAndSize(buffer, len);
             }
          }
          break;
    }
    return pValue;
}

PyObject * call_rmcios_as_python(PyObject *self, const struct context_rmcios *context, int id, PyObject *function, enum type_rmcios paramtype, int num_params, const union param_rmcios param)
{
    PyObject *pArgs;
    PyObject *pParams;
    PyObject *pValue;

    // Collect parameters to python object
    pArgs = PyTuple_New(4);
    pParams = PyTuple_New(num_params);
    int i;
    for (i = 0; i < num_params; i++)
    {
        pValue = param_to_python(context, paramtype, param, i);
        /* pValue reference stolen here: */
        PyTuple_SetItem(pParams, i, pValue);
    }
    
    PyTuple_SetItem(pArgs, 0, self); // self
    PyTuple_SetItem(pArgs, 1, PyLong_FromVoidPtr((void *)context)); // context
    PyTuple_SetItem(pArgs, 2, PyLong_FromLong(id)); // id
    PyTuple_SetItem(pArgs, 3, pParams);            // params
   
    // Call function:
    pValue = PyObject_CallObject(function, pArgs);
    if(pValue == 0) 
    {
       PyObject *ptype, *pvalue, *ptraceback;
       PyObject *pystr;
       char *str;

       PyErr_Fetch(&ptype, &pvalue, &ptraceback);
       pystr = PyObject_Str(pvalue);
       str = PyUnicode_AsUTF8(pystr);
       printf("failed call: %s!\n",str);
    }
    Py_DECREF(pArgs);
    return pValue;
}

void python_channel (PyObject *self,
                          const struct context_rmcios *context, int id,
                          enum function_rmcios function,
                          enum type_rmcios paramtype,
                          struct combo_rmcios *returnv,
                          int num_params, const union param_rmcios param)
{
    PyObject *callFunc = NULL;
    if (self == NULL) return;
    
    // Ensure calls from threads use GILState properly
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    switch (function)
    {
        case help_rmcios:
            callFunc = PyObject_GetAttrString(self, "help");
            break;
        case create_rmcios:
            callFunc = PyObject_GetAttrString(self, "create");
            break;
        case setup_rmcios:
            callFunc = PyObject_GetAttrString(self, "setup");
            break;
        case read_rmcios:
            callFunc = PyObject_GetAttrString(self, "read");
            break;
        case write_rmcios:
            callFunc = PyObject_GetAttrString(self, "write");
            break;
    }

    if (callFunc != NULL && PyCallable_Check(callFunc))
    {
        PyObject *ret = call_rmcios_as_python(self,
                                              context,
                                              id,
                                              callFunc, 
                                              paramtype,
                                              num_params, param);

        if(ret && ret != Py_BuildValue(""))
        {
           if(PyFloat_Check(ret))
           {
               float x = PyFloat_AsDouble(ret);
               return_float(context, returnv, x);
           }
           else if (PyLong_Check(ret))
           {
               int x = PyLong_AsLong(ret);
               return_int(context, returnv, x);
           }
           else if PyUnicode_Check(ret)
           {
               int length;
               char *s;
               s = PyUnicode_AsUTF8AndSize(ret, &length);
               if(s)
               {
                  return_buffer(context, returnv, s, length);
               }
           }
           else 
           {
                printf("Unknown type\n");
           }
        }
        else
        {
           printf("not ret value");
        }
        Py_XDECREF(callFunc);
    }
    else
    {
        printf("No function\n");
    }

    /* Release the thread. No Python API allowed beyond this point. */
    PyGILState_Release(gstate);
}

void python_module (PyObject *pModule,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    struct combo_rmcios *returnv,
                    int num_params, const union param_rmcios param)
{
   // Ensure calls from threads use GILState properly
   PyGILState_STATE gstate;
   gstate = PyGILState_Ensure();
   
   switch (function)
   {
      case help_rmcios:
       /* "create pymodule name test"
         "setup name channel1"
         "setup name channel2"*/
         break;
      case create_rmcios:
         if (num_params < 2) break;
    
         int blen = param_string_alloc_size(context, paramtype, param, 1);       
         {
            char buffer[blen];
            const char *module_name;
            PyObject * pName;
            module_name = param_to_string(context, paramtype, param, 1, blen, buffer);
            pName = PyUnicode_DecodeFSDefault(module_name);
            if(!pName) info (context, context->report, "NULL decode");
            pModule = PyImport_Import(pName);
            Py_DECREF(pName);
            
            if(!pModule) 
            {  
               PyObject *ptype, *pvalue, *ptraceback;
               PyObject *pystr;
               char *str;

               PyErr_Fetch(&ptype, &pvalue, &ptraceback);
               pystr = PyObject_Str(pvalue);
               str = PyUnicode_AsUTF8(pystr);
               printf("could not import module: %s %s\n", module_name, str);
            }
            create_channel_param (context, paramtype, param, 0, 
                                  (class_rmcios)python_module, pModule);
         }     
         break;
      case setup_rmcios:
         if (num_params < 1) 
         {
            break;
         }
         if (pModule == NULL){
            break;
   }
         else
         {
            int blen = param_string_alloc_size(context, paramtype, param, 0);
            {
               char buffer[blen];
               const char *channel_name;
               channel_name = param_to_string(context, paramtype, param, 0, blen, buffer);
               create_python_channel(context, pModule, channel_name);
            }
         }
         break;
   }

   /* Release the thread. No Python API allowed beyond this point. */
   PyGILState_Release(gstate);
}

void create_python_channel(const struct context_rmcios *context, PyObject *pModule, const char *class_name)
{
    // Ensure calls from threads use GILState properly
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject *this;    
    if (pModule) {
        this = PyObject_GetAttrString(pModule, class_name);
        if (!this)
        {
           info (context, context->report, "Could not load class");
           return;
        }

    }
    else 
    {
        info (context, context->report, "Invalid python module\n");
        return;
    }

    create_channel_str (context, class_name, (class_rmcios) python_channel, this);

    /* Release the thread. No Python API allowed beyond this point. */
    PyGILState_Release(gstate);
}

#ifdef INDEPENDENT_CHANNEL_MODULE
// function for dynamically loading the module
void API_ENTRY_FUNC init_channels (const struct context_rmcios *context)
{
   info (context, context->report,
         "python channels module\r\n[" VERSION_STR "]\r\n");

    Py_Initialize();
    PyEval_InitThreads();

    create_channel_str (context, "pymodule", (class_rmcios)python_module, 0);
}
#endif

