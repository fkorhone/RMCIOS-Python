import ctypes

# RMCIOS parameter type enumerations:
INT_RMCIOS = 1
FLOAT_RMCIOS = 2
BUFFER_RMCIOS = 3
CHANNEL_RMCIOS = 4
BINARY_RMCIOS = 5
COMBO_RMCIOS = 6

# RMCIOS function enumerations
HELP_RMCIOS = 1
SETUP_RMCIOS = 2
WRITE_RMCIOS = 3
READ_RMCIOS = 4
CREATE_RMCIOS = 5
LINK_RMCIOS = 6
functions_rmcios = [None, 'help_rmcios', 'setup_rmcios', 'write_rmcios', 'read_rmcios', 'create_rmcios', 'link_rmcios']

# Parameter structures:
class buffer_rmcios(ctypes.Structure):
    _fields_ = [("data", ctypes.POINTER(ctypes.c_char)),
                ("length", ctypes.c_int),
                ("size", ctypes.c_int),
                ("required_size", ctypes.c_int),
                ("trailing_size", ctypes.c_ushort)
                ]
                
class param_rmcios(ctypes.Union):
    _fields_ = [("pointer", ctypes.c_void_p),
                ("buffer_value", ctypes.POINTER(buffer_rmcios)),
                ("integer_value", ctypes.POINTER(ctypes.c_int)),
                ("combo_param", ctypes.c_void_p),
                ("channel", ctypes.c_int)
               ]

class combo_rmcios(ctypes.Structure):
    _fields_ = [("paramtype", ctypes.c_int),
                ("num_params", ctypes.c_int),
                ("param", ctypes.c_void_p),
                ("next", ctypes.c_void_p)
                ]

class CONTEXT(ctypes.Structure):
    _fields_ = [("version", ctypes.c_int),
        ("run_channel", ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p)),
        ("data", ctypes.c_void_p),
        ("id", ctypes.c_int),
        ("name", ctypes.c_int),
        ("mem", ctypes.c_int),
        ("quemem", ctypes.c_int),
        ("errors", ctypes.c_int),
        ("warning", ctypes.c_int),
        ("report", ctypes.c_int),
        ("control", ctypes.c_int),
        ("link", ctypes.c_int),
        ("linked", ctypes.c_int),
        ("create", ctypes.c_int),
        ("convert", ctypes.c_int)
        ]

