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

#ifndef PIP_CONVERT_H
#define PIP_CONVERT_H

#include <Python.h>
#include "zend.h"

/* PHP to Python */
PyObject * pip_hash_to_list(zval **hash);
PyObject * pip_hash_to_tuple(zval **hash);
PyObject * pip_hash_to_dict(zval **hash);
PyObject * pip_zobject_to_pyobject(zval **obj);
PyObject * pip_zval_to_pyobject(zval **val);

/* Python to PHP */
zval * pip_sequence_to_hash(PyObject *seq);
zval * pip_mapping_to_hash(PyObject *map);
zval * pip_pyobject_to_zobject(PyObject *obj);
zval * pip_pyobject_to_zval(PyObject *obj);

/* Arguments */
PyObject * pip_args_to_tuple(int argc, int start TSRMLS_DC);
PyObject * pip_args_to_tuple_ex(int ht, int argc, int start TSRMLS_DC);

/* Object Representations */

int pip_str(PyObject *obj, char **buffer, int *length);

#endif /* PIP_CONVERT_H */
