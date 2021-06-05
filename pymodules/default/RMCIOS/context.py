from RMCIOS.functions import *
from RMCIOS.API import *
import sys

def createPythonContext(context_address, redirect_stdout=True, redirect_stderr=True):
    python_context = CONTEXT.from_address(context_address)
    if redirect_stdout:
        sys.stdout = ChannelFile(python_context, python_context.report)
    if redirect_stderr:
        sys.stderr = ChannelFile(python_context, python_context.report)
    return python_context

