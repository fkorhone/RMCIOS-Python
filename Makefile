include RMCIOS-build-scripts/utilities.mk

SOURCES:=*.c
FILENAME:=python-module
CFLAGS+=-Ilinklib
CFLAGS+=libpython.a 
GCC?=gcc
DLLTOOL?=dlltool
MAKE?=make
INSTALLDIR:=..${/}..${/}
export

compile:
	$(DLLTOOL) -k --output-lib libpython.a --input-def linklib${/}python38.def
	$(MAKE) -f RMCIOS-build-scripts${/}module_dll.mk compile

install:
	${COPY} python-module.dll ${INSTALLDIR}modules
	${COPY} *.py ${INSTALLDIR}

