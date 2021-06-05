include RMCIOS-build-scripts/utilities.mk

SOURCES:=*.c
FILENAME:=python-module
CFLAGS+=-Ilinklib
GCC?=${TOOL_PREFIX}gcc
DLLTOOL?=${TOOL_PREFIX}dlltool
MAKE?=make
INSTALLDIR:=..${/}..
PYLIBDIR = pymodules${/}default${/}RMCIOS
export

compile:
	$(MAKE) -f RMCIOS-build-scripts${/}module_dll.mk compile TOOL_PREFIX=${TOOL_PREFIX}

install:
	-${MKDIR} "${INSTALLDIR}${/}modules"
	${COPY} python-module.dll ${INSTALLDIR}${/}modules
	-${MKDIR} "${INSTALLDIR}${/}${PYLIBDIR}"
	${COPY} ${PYLIBDIR}${/}*.py ${INSTALLDIR}${/}${PYLIBDIR}

