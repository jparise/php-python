/*
 * Python in PHP - Embedded Python Extension
 *
 * Copyright (c) 2003,2004,2005,2006,2007,2008 Jon Parise <jon@php.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * $Id$
 */

#ifndef PHP_PYTHON_INTERNAL_H
#define PHP_PYTHON_INTERNAL_H

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4005 4142)
#endif

#include "Python.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "zend.h"

/*
 * Py_ssize_t was introduced in Python 2.5.  Define our own version for
 * compatiblity with earlier versions.
 */
#ifndef Py_ssize_t
	#define Py_ssize_t ssize_t
#endif

ZEND_BEGIN_MODULE_GLOBALS(python)
    PyThreadState *tstate;
ZEND_END_MODULE_GLOBALS(python)

#ifdef ZTS
#define PYG(v) TSRMG(python_globals_id, zend_python_globals *, v)
#else
#define PYG(v) (python_globals.v)
#endif

typedef struct _php_python_object {
	zend_object			base;
	PyObject *			object;
	zend_class_entry *	ce;
} php_python_object;

#define PHP_PYTHON_FETCH(name, zv) php_python_object *name = (php_python_object *)zend_object_store_get_object(zv TSRMLS_CC)
#define PHP_PYTHON_THREAD_ASSERT() assert(PyThreadState_GET() == PYG(tstate))
#define PHP_PYTHON_THREAD_ACQUIRE() PyEval_AcquireThread(PYG(tstate))
#define PHP_PYTHON_THREAD_RELEASE() PyEval_ReleaseThread(PyThreadState_GET())

/* Python Modules */
int python_php_init(); 

/* PHP Object API */
zend_object_value python_object_create(zend_class_entry *ce TSRMLS_DC);
zend_uint python_get_arg_info(PyObject *callable, zend_arg_info **arg_info TSRMLS_DC) ;

/* PHP to Python Conversion */
PyObject * pip_hash_to_list(zval *hash TSRMLS_DC);
PyObject * pip_hash_to_tuple(zval *hash TSRMLS_DC);
PyObject * pip_hash_to_dict(zval *hash TSRMLS_DC);
PyObject * pip_zobject_to_pyobject(zval *obj TSRMLS_DC);
PyObject * pip_zval_to_pyobject(zval *val TSRMLS_DC);

/* Python to PHP Conversion */
int pip_sequence_to_hash(PyObject *o, HashTable *ht TSRMLS_DC);
int pip_sequence_to_array(PyObject *o, zval *zv TSRMLS_DC);
int pip_mapping_to_hash(PyObject *o, HashTable *ht TSRMLS_DC);
int pip_mapping_to_array(PyObject *o, zval *zv TSRMLS_DC);
int pip_pyobject_to_zobject(PyObject *o, zval *zv TSRMLS_DC);
int pip_pyobject_to_zval(PyObject *o, zval *zv TSRMLS_DC);

/* Argument Conversion */
PyObject * pip_args_to_tuple(int argc, int start TSRMLS_DC);

/* Object Representations */
int python_str(PyObject *o, char **buffer, int *length TSRMLS_DC);

#endif /* PHP_PYTHON_INTERNAL_H */
