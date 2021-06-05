import traceback
from RMCIOS.API import *

CHFUNC = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p)

# array to keep callback references alive. We don't want funcs to be garbage collected.
funcrefs = []

class Param:
    # TODO
    def __init__(self, context, paramtype, param, index):
        self.context = context
        self.paramtype = paramtype
        self.param = param
        self.index = index
    
    def __int__(self):
        value = ctypes.c_int(0)
        params = combo_rmcios(INT_RMCIOS, 1, ctypes.addressof(value), 0) 
        self.context.run_channel(self.context.data, ctypes.addressof(self.context), self.context.convert, WRITE_RMCIOS, self.paramtype, ctypes.pointer(params), self.index + 1, self.param)
    
        return value.value

    def __float__(self):
        value = ctypes.c_float(0)
        params = combo_rmcios(FLOAT_RMCIOS, 1, ctypes.addressof(value), 0)
        self.context.run_channel(self.context.data, ctypes.addressof(self.context), self.context.convert, WRITE_RMCIOS, self.paramtype, ctypes.pointer(params), self.index + 1, self.param)
        return value.value

    def _to_buffer(self):
        # Get required size:

        params = combo_rmcios(BUFFER_RMCIOS, 1, 0, 0) 
        self.context.run_channel(self.context.data, ctypes.addressof(self.context), self.context.convert, READ_RMCIOS, self.paramtype, ctypes.pointer(params), self.index + 1, self.param)
        buff = buffer_rmcios.from_address(params.param)

        # Convert to string:
        cBuffer = ctypes.create_string_buffer( buff.required_size )
        buff = buffer_rmcios(data = cBuffer,
                         length = 0,
                         size = buff.required_size,
                         required_size = 0,
                         trailing_size = 0)
        params = combo_rmcios(BUFFER_RMCIOS, 1, ctypes.addressof(buff), 0) 
        self.context.run_channel(self.context.data, ctypes.addressof(self.context), self.context.convert, WRITE_RMCIOS, self.paramtype, ctypes.pointer(params), self.index + 1, self.param)
        return cBuffer.value

    def __str__(self):
        return self._to_buffer().decode('ascii')

    def __iter__(self):
        self.buffer_data = self._to_buffer()
        self.iterate_index = -1
        return self

    def __next__(self):
        self.iterate_index += 1
        if self.iterate_index < len(self.buffer_data):
            return self.buffer_data[self.iterate_index]
        else:
            raise StopIteration 

def ch_func(data, context_addr, id, function, paramtype, returnv, num_params, param):
      # Convert data pointer to python object
      cpobject = ctypes.cast(data, ctypes.POINTER(ctypes.py_object))
      python_object = cpobject.contents.value

      # Convert context pointer to python context object
      python_context = CONTEXT.from_address(context_addr)

      # convert prameters to python list with python objects
      pyparams = []
      for index in range(num_params):
        pyparams.append(Param(python_context, paramtype, param, index))

      # Look for function to execute from python object:
      try:
        name = functions_rmcios[function]
        func = getattr(python_object, name)
        func(python_object, python_context, id, *pyparams)
      except:
        print("Error executing python object member function")
        traceback.print_exc() 
      
      return

def create_python_channel(context, name, data):
      namebuffer = ctypes.create_string_buffer(name.encode())
     
      # Create c function pointer and convert it to generic c buffer (char *)
      channel = CHFUNC(ch_func)
      funcrefs.append(channel)
      func_as_c_buffer = ctypes.cast(ctypes.pointer(channel), ctypes.POINTER(ctypes.c_char))
     
      # Create c data pointer and convert it to generic c buffer (char *)
      pobject = ctypes.pointer(ctypes.py_object(data))
      ptr_as_c_buffer = ctypes.cast( ctypes.pointer(pobject), ctypes.POINTER(ctypes.c_char)) 
      
      # Create array of binary parameters  for channel execution:
      buffers_type = buffer_rmcios * 2
      buffers = buffers_type( ( func_as_c_buffer, ctypes.sizeof(channel), 0, ctypes.sizeof(channel), 0),
                              ( ptr_as_c_buffer, ctypes.sizeof(pobject), 0, ctypes.sizeof(pobject), 0) )

      # Create buffer name 
      name_buffer = buffer_rmcios(namebuffer, len(name), 0, len(name), 0)

      # create combo param for returnv and params for creating name
      new_channel_id = ctypes.c_int(0)
      combo_array_type = combo_rmcios * 2
      params = combo_array_type( (INT_RMCIOS, 1, ctypes.addressof(new_channel_id),0), 
                                 (BUFFER_RMCIOS, 1, ctypes.addressof(name_buffer),0))

      # Call channel context.create to create the new channel
      context.run_channel(context.data, ctypes.addressof(context), context.create, CREATE_RMCIOS, BINARY_RMCIOS, params, 2, buffers)

      # Add name for the channel
      if len(name) > 0:
        context.run_channel(context.data, ctypes.addressof(context), context.name, WRITE_RMCIOS, COMBO_RMCIOS, 0, 2, params)
      return new_channel_id

def run_channel(context, channel, function, *args):
    #params = combo_rmcios() * len(args)
    len(args)
    pure_ints = True
    pure_floats = True
    pure_buffers = True

    for arg in args:
        if isinstance(arg, int):
            pure_floats = False
            pure_buffers = False

        if isinstance(arg, float):
            pure_ints = False
            pure_buffers = False

        if isinstance(arg, str):
            pure_ints = False
            pure_floats = False

        if isinstance(arg, (bytes, bytearray)):
            pure_ints = False
            pure_floats = False

    if(pure_ints):
        intargtype = ctypes.c_int * len(args)
        return context.run_channel(context.data, ctypes.addressof(context), channel, function, INT_RMCIOS, 0, len(args), intargtype(*args))

    if(pure_floats):
        floatargtype = ctypes.c_float * len(args)
        return context.run_channel(context.data, ctypes.addressof(context), channel, function, FLOAT_RMCIOS, 0, len(args), floatargtype(*args))
    
    if(pure_buffers):
        buffersargtype = buffer_rmcios * len(args)
        buffers = []
        for arg in args:
            cBuffer = ctypes.create_string_buffer(arg.encode())
            buffers.append((cBuffer, len(arg), 0, len(arg), 0))
        
        params=buffersargtype(*buffers)
        return context.run_channel(context.data, ctypes.addressof(context), channel, function, BUFFER_RMCIOS, 0, len(args), ctypes.pointer(params))
    
    # This is a combined parameter type call
    combosArgType = combo_rmcios * len(args)
    comboArguments = []
    argstore = []
    intArguments = []
    for arg in args:
        if isinstance(arg, int):
            argstore.append(ctypes.c_int(arg))
            comboArguments.append((1, 1, ctypes.addressof(argstore[-1]), None))

        if isinstance(arg, float):
            argstore.append(ctypes.c_float(arg))
            comboArguments.append((FLOAT_RMCIOS, 1, ctypes.addressof(argstore[-1]), None))

        if isinstance(arg, str) or isinstance(arg, (bytes, bytearray)):
            cBuffer = ctypes.create_string_buffer(arg.encode())
            param = buffer_rmcios(data = cBuffer,
                                  length = len(arg),
                                  size = 0,
                                  required_size = len(arg),
                                  trailing_size = 0)
            argstore.append(param)
            comboArguments.append((BUFFER_RMCIOS, 1, ctypes.addressof(argstore[-1]), None))

    params = combosArgType(*comboArguments)
    return context.run_channel(context.data, ctypes.addressof(context), channel, function, COMBO_RMCIOS, 0, len(args), ctypes.pointer(params))

def help_channel(context, channel, *args):
    return run_channel(context, channel, HELP_RMCIOS, *args)

def setup_channel(context, channel, *args):
    return run_channel(context, channel, SETUP_RMCIOS, *args)

def read_channel(context, channel, *args):
    return run_channel(context, channel, READ_RMCIOS, *args)

def write_channel(context, channel, *args):
    return run_channel(context, channel, WRITE_RMCIOS, *args)

class ChannelFile(object):
    def __init__(self, context, channel):
        self.python_context = context
        self.channel = channel

    def write(self, message):
        write_channel(self.python_context, self.channel, message)
        pass    

    def flush(self):
        pass


