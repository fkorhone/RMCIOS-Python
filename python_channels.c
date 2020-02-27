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

struct python_channel_data {
   PyObject *pModule;
   PyObject *pChannel;
   PyObject *pHelp;
   PyObject *pCreate;
   PyObject *pSetup;
   PyObject *pRead;
   PyObject *pWrite;
};

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

PyObject * call_rmcios_as_python(const struct context_rmcios *context, PyObject *function, enum type_rmcios paramtype, struct combo_rmcios *returnv, int num_params, const union param_rmcios param)
{
    PyObject *pArgs;
    PyObject *pParams;

    PyObject *pValue;

    // Collect parameters to python objects
    pArgs = PyTuple_New(5);
    pParams = PyTuple_New(num_params);
    int i;
    for (i = 0; i < num_params; i++)
    {
        pValue = param_to_python(context, paramtype, param, i);
        /* pValue reference stolen here: */
        PyTuple_SetItem(pParams, i, pValue);
    }
    
    PyTuple_SetItem(pArgs, 0, PyLong_FromLong(0)); // self
    PyTuple_SetItem(pArgs, 1, PyLong_FromLong(0)); // context
    PyTuple_SetItem(pArgs, 2, PyLong_FromLong(0)); // id
    PyTuple_SetItem(pArgs, 3, pParams);            // params
   
    // Call function:
    printf("calling");
    pValue = PyObject_CallObject(function, pArgs);
    Py_DECREF(pArgs);
    if(pValue == 0) printf("failed call!\n");
    return pValue;
}

void python_channel (struct python_channel_data *this,
                          const struct context_rmcios *context, int id,
                          enum function_rmcios function,
                          enum type_rmcios paramtype,
                          struct combo_rmcios *returnv,
                          int num_params, const union param_rmcios param)
{
    PyObject *callFunc = NULL;
    if (this == NULL) return;

    switch (function)
    {
        case help_rmcios:
            callFunc = this->pHelp;
            break;
        case create_rmcios:
            callFunc = this->pCreate;
            break;
        case setup_rmcios:
            callFunc = this->pSetup;
            break;
        case read_rmcios:
            callFunc = this->pRead;
            break;
        case write_rmcios:
            callFunc = this->pWrite;
            break;
    /*
       case delete:
       Py_XDECREF(this->pHelp);
       Py_XDECREF(this->pCreate);
       Py_XDECREF(this->pSetup);
       Py_XDECREF(this->pRead);
       Py_XDECREF(this->pWrite);
       break;
       */
    }

    if (callFunc != NULL && PyCallable_Check(callFunc))
    {
        PyObject *ret = call_rmcios_as_python(context, 
                              callFunc, 
                              paramtype, returnv,
                              num_params, param);
        if(ret)// && ret != Py_BuildValue(""))
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
           else{
                printf("Unknown type\n");
           }
        }
        else
        {
           printf("not ret value");
        }
    }
    else
    {
        printf("No function\n");
    }
}

void python_module (PyObject *pModule,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    struct combo_rmcios *returnv,
                    int num_params, const union param_rmcios param)
{
   switch (function)
   {
      case help_rmcios:
       /* "create pymodule name test"
         "setup name channel1"
         "setup name channel2"*/
         break;
      case create_rmcios:
            printf("create");
         if (num_params < 2) break;
    
         if (Py_IsInitialized() == 0)
         {
            Py_Initialize();
         }
         int blen = param_string_alloc_size(context, paramtype, param, 1);       
         {
            char buffer[blen];
            const char *module_name;
            PyObject * pName;
            module_name = param_to_string(context, paramtype, param, 1, blen, buffer);
            pName = PyUnicode_DecodeFSDefault(module_name);
            if(!pName) info (context, context->report, "NULL decode");
            pModule = PyImport_Import(pName);
            if(!pModule) printf("could not import module: %s\n", module_name);
            create_channel_param (context, paramtype, param, 0, 
                                  (class_rmcios)python_module, pModule);
         }     
         break;
      case setup_rmcios:
            printf("setup\n");
         if (num_params < 1) 
         {
            printf("params");
            break;
         }
         if (pModule == NULL){
            printf("NULL\n");
            break;
   }
         else
         {
            printf("setting up\n");
            int blen = param_string_alloc_size(context, paramtype, param, 0);
            {
               char buffer[blen];
               const char *channel_name;
               channel_name = param_to_string(context, paramtype, param, 0, blen, buffer);
               printf("creating:%s\n",channel_name);
               create_python_channel(context, pModule, channel_name);
            }
         }
         break;
   }
}

void create_python_channel(const struct context_rmcios *context, PyObject *pModule, const char *class_name)
{
    struct python_channel_data *this;    
    PyObject * pName;
    this = allocate_storage (context, sizeof (struct python_channel_data), 0);  
    this->pModule=pModule;
    if (this->pModule) {
        this->pChannel = PyObject_GetAttrString(this->pModule, class_name);
        if (!this->pChannel) 
        {
           info (context, context->report, "Could not load class");
           return;
        }
        this->pHelp   = PyObject_GetAttrString(this->pChannel, "help");
        if(this->pHelp==0) printf("NO HELP\n");
        this->pCreate = PyObject_GetAttrString(this->pChannel, "create");
        this->pSetup  = PyObject_GetAttrString(this->pChannel, "setup");
        this->pRead   = PyObject_GetAttrString(this->pChannel, "read");
        this->pWrite  = PyObject_GetAttrString(this->pChannel, "write");
    }
    else 
    {
        info (context, context->report, "Invalid python module\n");
        return;
    }

    create_channel_str (context, class_name, (class_rmcios) python_channel, this);
}

#ifdef INDEPENDENT_CHANNEL_MODULE
// function for dynamically loading the module
void API_ENTRY_FUNC init_channels (const struct context_rmcios *context)
{
   info (context, context->report,
         "python channels module\r\n[" VERSION_STR "]\r\n");
    create_channel_str (context, "pymodule", (class_rmcios)python_module, 0);
}
#endif

