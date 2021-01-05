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
#include <Python.h>
#include <stdio.h>

void python_module (void *data,
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
         /*
         "write python code\n"
         */
         break;
      
      case write_rmcios:

        if(num_params < 1)
        {
            break;
        }
        else if(num_params == 1)
        {
            char cmd[64]; 
            sprintf(cmd,"context=0x%p\n", context);
            PyRun_SimpleString(cmd);
            
            int blen = param_string_alloc_size(context, paramtype, param, 0);
            {
                char buffer[ blen ];
                const char *s = param_to_string(context, paramtype, param, 0, blen, buffer);
                PyRun_SimpleString(s);
            }
        }
        else
        {
            char cmd[64]; 
            sprintf(cmd,"context=0x%p\n", context);
            PyRun_SimpleString(cmd);
            
            // Combine parameters into single string
            int i;
            int lengths[num_params];
            int tot_length = 0;
            for(i = 0; i < num_params; i++)
            {
                lengths[i] = param_string_length(context, paramtype, param, i);
                tot_length += lengths[i];
            }
            tot_length += num_params + 1; // Space for whitespace between params, newline and terminating 0   

            {
                char buffer[tot_length];
                int pos=0; 

                for(i = 0; i < num_params; i++)
                {
                    int plen = lengths[i];
                    param_to_string(context, paramtype, param, i, tot_length - pos, buffer + pos);
                    buffer[pos + plen] = ' ';
                    pos += plen + 1;
                }

                buffer[tot_length - 2] = '\n';
                buffer[tot_length - 1] = 0;

                PyRun_SimpleString(buffer);
            }
        }
        break;
   }

   /* Release the thread. No Python API allowed beyond this point. */
   PyGILState_Release(gstate);
}

#ifdef INDEPENDENT_CHANNEL_MODULE
// function for dynamically loading the module
void API_ENTRY_FUNC init_channels (const struct context_rmcios *context)
{
    info (context, context->report, "python channels module\r\n[" VERSION_STR "]\r\n");

    Py_Initialize();
    PyEval_InitThreads();
    create_channel_str (context, "python", (class_rmcios)python_module, NULL);
}
#endif

