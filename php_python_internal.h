/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Jon Parise <jon@php.net>                                     |
   +----------------------------------------------------------------------+
 */

 /* $Id$ */

#ifndef PHP_PYTHON_INTERNAL_H
#define PHP_PYTHON_INTERNAL_H

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4005)
#endif

#include "Python.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "zend.h"

typedef struct _php_python_object {
	PyObject *			object;
    zend_class_entry *	ce;
} php_python_object;

#define PIP_FETCH(zv)	(php_python_object *)zend_object_store_get_object(zv TSRMLS_CC)

zend_object_value python_object_create(zend_class_entry *ce TSRMLS_DC);
void python_object_destroy(void *object, zend_object_handle handle TSRMLS_DC);
void python_object_clone(void *object, void **clone_ptr TSRMLS_DC);

zend_object_handlers python_object_handlers;

PHP_FUNCTION(python_new);

/* PHP to Python Conversion */
PyObject * pip_hash_to_list(zval **hash);
PyObject * pip_hash_to_tuple(zval **hash);
PyObject * pip_hash_to_dict(zval **hash);
PyObject * pip_zobject_to_pyobject(zval **obj);
PyObject * pip_zval_to_pyobject(zval **val);

/* Python to PHP Conversion */
zval * pip_sequence_to_hash(PyObject *seq);
zval * pip_mapping_to_hash(PyObject *map);
zval * pip_pyobject_to_zobject(PyObject *obj);
zval * pip_pyobject_to_zval(PyObject *obj);

/* Argument Conversion */
PyObject * pip_args_to_tuple(int argc, int start TSRMLS_DC);
PyObject * pip_args_to_tuple_ex(int ht, int argc, int start TSRMLS_DC);

/* Object Representations */

int pip_str(PyObject *obj, char **buffer, int *length);

#endif /* PHP_PYTHON_INTERNAL_H */
