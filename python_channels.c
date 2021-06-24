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
#include <wtypes.h>
#include <wchar.h>
#include <stdlib.h>

static initialized = 0; 

HINSTANCE hDLLInstance;
loadPythonLibary(const char *shared_library)
{
    printf("Loading library %s\n", shared_library);
    hDLLInstance = LoadLibrary(shared_library);
    if (hDLLInstance)
    {
        //If successfully loaded, get the address of the desired functions.
        Py_Initialize = GetProcAddress (hDLLInstance, "Py_Initialize");
        PyEval_InitThreads = GetProcAddress (hDLLInstance, "PyEval_InitThreads");
        PyGILState_Ensure = GetProcAddress (hDLLInstance, "PyGILState_Ensure");
        PyGILState_Release = GetProcAddress (hDLLInstance, "PyGILState_Release");
        PyRun_SimpleString = GetProcAddress (hDLLInstance, "PyRun_SimpleString");
        Py_SetPythonHome = GetProcAddress (hDLLInstance, "Py_SetPythonHome");
        Py_SetPath = GetProcAddress (hDLLInstance, "Py_SetPath");
        Py_DecodeLocale = GetProcAddress (hDLLInstance, "Py_DecodeLocale");
    }
    else
    {
        printf ("Failed to load %s\n", shared_library);
    }
}

struct python_data {
    int python_version_major;
    int python_version_minor;
    const char *basepath;
    const char *pythonpath;
    const char *pythonhome;
    const char *shared_library;
    const char *context_variable;
};
const char * default_pythonlib = "python38.dll";
const char * default_pythonpath = "pymodules\\default";
const char * default_pythonhome = "python\\default";
const char * default_context_variable = "context";

void python_module (void *data,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    struct combo_rmcios *returnv,
                    int num_params, const union param_rmcios param)
{
   struct python_data *self = data;
   switch (function)
   {
      case help_rmcios:
        return_string (context, returnv,
                       "python channel\n"
                       "create python newname | basedir | python_home(python/default) | python_library(python38.dll)\n" 
                       "    Create new python interperter. Optionally specify channel to basedir, string to python home, and shared library name.\n"
                       "setup newname | context_variable(context) | python_path(pymodules/default/) \n"
                       "    Context address variable name and optionally python module search path to be added\n"
                       "    Variable named by context_variable parameter is added to the executed python code\n"
                       "    Passed variable enables python code capability to interact with RMCIOS system\n"
                       "    Disable context_variable by setting it to value 0\n"
                       "setup python\n"
                       "    Read python configuration\n"
                       "write python command\n"
                       "    Execute python command with the interpereter\n"
                       "    Multiple command parameters are joined together to form unified python code\n"
                       "    Each write are given to python directly. Therefore writes need to represent execute ready python code (not part of it)\n"
                       );
         break;
      case create_rmcios:
        {
            if (num_params < 1) break;

            self = (struct python_data *) allocate_storage (context, sizeof (struct python_data), 0);       
            self->context_variable = default_context_variable; 
            self->pythonpath = default_pythonpath; 
            self->basepath = 0;
            self->shared_library = default_pythonlib;
            self->pythonhome = default_pythonhome;
        
            int base_path_channel = 0;
            if (num_params > 1)
            {
                base_path_channel = param_to_channel(context, paramtype, param, 1);
            }
            else if(!getenv("PYTHONHOME"))
            {
                base_path_channel = channel_enum(context, "installpath");
            }
        
            if(base_path_channel) 
            {
                if(base_path_channel != 0) {
                    int baselen = read_str(context, base_path_channel, 0 , 0); // Read required size
                    self->basepath = allocate_storage (context, baselen+1, 0); 
                    if(!self->basepath) break;
                    read_str(context, base_path_channel, self->basepath, baselen+1); // Read required size
                }
                else if (num_params > 1){ 
                    int baselen = param_string_length(context, paramtype, param, 1);
                    self->basepath = allocate_storage (context, baselen+1, 0); 
                    if(!self->basepath) break;
                    param_to_string(context, paramtype, param, 1, baselen+1, self->basepath);
                }
            }

            if(num_params >= 2){
                int homelen = param_string_length(context, paramtype, param, 2);
                self->pythonhome = allocate_storage (context, homelen+1, 0); 
                param_to_string(context, paramtype, param, 2, homelen+1, self->pythonhome);
            }        
    
            if(num_params >= 3){
                int liblen = param_string_length(context, paramtype, param, 3);
                self->shared_library = allocate_storage (context, liblen+1, 0); 
                if(!self->shared_library) break;
                param_to_string(context, paramtype, param, 3, liblen+1, self->shared_library);
            } 

            create_channel_param (context, paramtype, param, 0, 
                                  (class_rmcios) python_module, self); 
        }
        //PyThreadState * interprtr = Py_NewInterpreter();
        //PyThreadState_Swap(interprt);
        //Py_EndInterpreter(interprt);
        break;

      case setup_rmcios:
        {
            char *pythonpath = default_pythonpath;
            char *context_variable = default_context_variable;

            if(num_params >= 1){
                int variablelen = param_string_length(context, paramtype, param, 0);
                char *variable = allocate_storage (context, variablelen+1, 0); 
                if(!variable) break;
                param_to_string(context, paramtype, param, 0, variablelen+1, variable);
                self->context_variable = variable;
            }

            if(num_params >= 2){
                int pythonpath_len = param_string_length(context, paramtype, param, 1);
                char pythonpath_storage = allocate_storage (context, pythonpath_len+1, 0); 
                if(!pythonpath_storage) break;
                param_to_string(context, paramtype, param, 1, pythonpath_len+1, pythonpath_storage);
                self->pythonpath = pythonpath_storage;
            }
        }
        break;
      case write_rmcios:
        if(initialized == 0)
        {
            const wchar_t * wc_python_home = 0;
            const wchar_t * wc_python_path = 0;
            
            if(self->basepath && self->pythonhome) 
            {
                int liblen = strlen(self->basepath) + strlen(self->pythonhome) + strlen(self->shared_library);
                char shared_library[liblen+1];
                strcpy(shared_library, self->basepath);
                strcat(shared_library, self->pythonhome);
                strcat(shared_library, self->shared_library);
                loadPythonLibary(shared_library);

                strcpy(shared_library, self->basepath);
                strcat(shared_library, self->pythonhome);
                wc_python_home = Py_DecodeLocale(shared_library, 0);
            }        
            else
            {
                const char* env_pythonhome = getenv("PYTHONHOME");
                int liblen = strlen(env_pythonhome) + strlen(self->shared_library);
                char shared_library[liblen+1];
                strcpy(shared_library, env_pythonhome);
                strcat(shared_library, self->shared_library);
                loadPythonLibary(shared_library);
            }

            int pathlen = 2 * strlen(self->basepath) + strlen(self->pythonpath) + 1 + strlen(self->pythonhome);
            char pythonpath[pathlen+1];
            strcpy(pythonpath, self->basepath);
            strcat(pythonpath, self->pythonpath);
            strcat(pythonpath, ";");
            strcat(pythonpath, self->basepath);
            strcat(pythonpath, self->pythonhome);
            wc_python_path = Py_DecodeLocale(pythonpath, 0);
            
            Py_SetPath(wc_python_path);
            if(wc_python_home) {
                Py_SetPythonHome(wc_python_home);
            }
            Py_Initialize();
            PyEval_InitThreads(); 
            initialized = 1;
        }
        if(function == setup_rmcios) break;

        if(num_params < 1)
        {
            break;
        }
        else if(num_params == 1)
        {
            // Ensure calls from threads use GILState properly
            PyGILState_STATE gstate;
            gstate = PyGILState_Ensure();
            
            char cmd[64]; 
            sprintf(cmd,"%s=0x%p\n", self->context_variable, context);
            PyRun_SimpleString(cmd);
            
            int blen = param_string_alloc_size(context, paramtype, param, 0);
            {
                char buffer[ blen ];
                const char *s = param_to_string(context, paramtype, param, 0, blen, buffer);
                PyRun_SimpleString(s);
            }
            
            /* Release the thread. No Python API allowed beyond this point. */
            PyGILState_Release(gstate);
        }
        else
        {
            // Ensure calls from threads use GILState properly
            PyGILState_STATE gstate;
            gstate = PyGILState_Ensure();
            
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
            /* Release the thread. No Python API allowed beyond this point. */
            PyGILState_Release(gstate);
        }
        break;
   }

}
    
struct python_data default_python = {
        .python_version_major = 3,
        .python_version_minor = 8,
        .shared_library = "python38.dll",
        .basepath="",
        .pythonpath = "pymodules\\default",
        .pythonhome = "python\\default",
        .context_variable = "context"
};

#ifdef INDEPENDENT_CHANNEL_MODULE
// function for dynamically loading the module
void API_ENTRY_FUNC init_channels (const struct context_rmcios *context)
{
    info (context, context->report, "python channels module\r\n[" VERSION_STR "]\r\n");
    create_channel_str (context, "python", (class_rmcios)python_module, &default_python);
}
#endif

