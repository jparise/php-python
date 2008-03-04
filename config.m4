dnl $Id$
dnl config.m4 for python extension

PHP_ARG_WITH(python, for embedded Python support,
[  --with-python           Include embedded Python support])

if test "$PHP_PYTHON" != "no"; then

    if test "$ext_shared" = "shared" || test "$ext_shared" = "yes"; then
        AC_PATH_PROG([PYTHON_CONFIG],[python-config])
    else
        AC_PATH_PROG([PYTHON_CONFIG],[python-shared-config])
    fi

    if test -z "$PYTHON_CONFIG"; then
        AC_MSG_RESULT(not found)
        AC_MSG_ERROR([Cannot find python-config in your system path])
    fi

    AC_MSG_CHECKING(for Python includes)
    PYTHON_CFLAGS=`$PYTHON_CONFIG --includes`
    AC_MSG_RESULT($PYTHON_CFLAGS)

    AC_MSG_CHECKING(for Python libraries)
    PYTHON_LDFLAGS=`$PYTHON_CONFIG --ldflags`
    AC_MSG_RESULT($PYTHON_LDFLAGS)

    AC_CHECK_LIB(pthread, pthread_create, [
        PYTHON_LDFLAGS="-lpthread $PYTHON_LDFLAGS"
    ])
    AC_CHECK_LIB(dl, dlopen, [
        PYTHON_LDFLAGS="-ldl $PYTHON_LDFLAGS"
    ])

    AC_MSG_CHECKING([consistency of Python build environment])
    AC_LANG_PUSH([C])
    LDFLAGS="$ac_save_LDFLAGS $PYTHON_LDFLAGS"
    CFLAGS="$ac_save_CFLAGS $PYTHON_CFLAGS"
    AC_TRY_LINK([
            #include <Python.h>
    ],[
            Py_Initialize();
    ],[pythonexists=yes],[pythonexists=no])

    AC_MSG_RESULT([$pythonexists])

    if test ! "$pythonexists" = "yes"; then
        AC_MSG_ERROR([
  Could not link test program to Python. Maybe the main Python library has been
  installed in some non-standard library path. If so, pass it to configure,
  via the LDFLAGS environment variable.
  Example: ./configure LDFLAGS="-L/usr/non-standard-path/python/lib"
  ============================================================================
   ERROR!
   You probably have to install the development version of the Python package
   for your distribution.  The exact name of this package varies among them.
  ============================================================================
        ])
    fi
    AC_LANG_POP
    CFLAGS="$ac_save_CFLAGS"
    LDFLAGS="$ac_save_LDFLAGS"

    PHP_EVAL_INCLINE($PYTHON_CFLAGS)
    PHP_EVAL_LIBLINE($PYTHON_LDFLAGS, PYTHON_SHARED_LIBADD)
    PHP_SUBST(PYTHON_SHARED_LIBADD)

    PHP_NEW_EXTENSION(python, python.c python_convert.c python_handlers.c python_object.c python_php.c, $ext_shared)
fi
