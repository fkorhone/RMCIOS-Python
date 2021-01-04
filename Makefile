include RMCIOS-build-scripts/utilities.mk

SOURCES:=*.c
FILENAME:=python-module
CFLAGS+=-Ilinklib${/}
CFLAGS+=libpython.a 
GCC?=${TOOL_PREFIX}gcc
DLLTOOL?=${TOOL_PREFIX}dlltool
MAKE?=make
INSTALLDIR:=..${/}..${/}
export

compile:
	$(DLLTOOL) -k --output-lib libpython.a --input-def linklib${/}python38.def
	$(MAKE) -f RMCIOS-build-scripts${/}module_dll.mk compile TOOL_PREFIX=${TOOL_PREFIX}

install:
	${MKDIR} ${INSTALLDIR}modules
	${COPY} python-module.dll ${INSTALLDIR}modules
	${COPY} *.py ${INSTALLDIR}

