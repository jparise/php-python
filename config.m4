dnl $Id$
dnl config.m4 for extension python

PHP_ARG_WITH(python, for embedded Python support,
[  --with-python[=DIR]     Include embedded Python support])

if test "$PHP_PYTHON" != "no"; then
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="/include/python2.2/Python.h"
  if test -r $PHP_PYTHON/; then
     PYTHON_DIR=$PHP_PYTHON
  else # search default path list
     AC_MSG_CHECKING(for Python files in default path)
     for i in $SEARCH_PATH ; do
       if test -r $i/$SEARCH_FOR; then
         PYTHON_DIR=$i
         AC_MSG_RESULT(found in $i)
       fi
     done
  fi
 
  if test -z "$PYTHON_DIR"; then
     AC_MSG_RESULT(not found)
     AC_MSG_ERROR(Please specify path to Python distribution files)
  fi

  dnl # --with-python -> chech for lib and symbol presence
  LIBNAME=python2.2
  LIBSYMBOL=Py_Initialize
  old_LIBS=$LIBS
  LIBS="$LIBS -L$PYTHON_DIR/lib/python2.2/config -lm -pthread -lutil"
  AC_CHECK_LIB($LIBNAME, $LIBSYMBOL, [AC_DEFINE(HAVE_PYTHONLIB,1,[ ])],
		[AC_MSG_ERROR(wrong Python lib version or lib not found)])
  LIBS=$old_LIBS
  
  PHP_SUBST(PYTHON_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $PYTHON_DIR/lib/python2.2/config, PYTHON_SHARED_LIBADD)
  PHP_ADD_LIBRARY(m)
  PHP_ADD_LIBRARY(util)
  LIBS="$LIBS -pthread"

  PHP_ADD_INCLUDE($PYTHON_DIR/include/python2.2)
  PHP_EXTENSION(python, $ext_shared)
fi
