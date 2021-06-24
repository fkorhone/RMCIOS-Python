# RMCIOS-Python
Python module for RMCIOS

## Clone subrepositories
Before compiling from sources you need to clone subrepostiories:
git submodule update --init --recursive

## Compiling
There is makefile for compiling.
make
And shared object (.dll on windows will be created)

## Python search logic

By default module looks for PYTHONHOME environment variable. 
If PYTHONHOME variable is not found module will read basepath from channel named installpath and append python/default to it.
Default PYTHONHOME and PYTHONPATH can also be overriden by giving them as parameters to the python interperter channel.


