/*
 * Python in PHP - Embedded Python Extension
 *
 * Copyright (c) 2003,2004,2005,2006,2007 Jon Parise <jon@php.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "php.h"
#include "php_python_internal.h"

extern zend_object_handlers python_object_handlers;

/* {{{ python_object_destroy(void *object, zend_object_handle handle TSRMLS_DC)
 */
static void
python_object_destroy(void *object, zend_object_handle handle TSRMLS_DC)
{
	php_python_object *pip = (php_python_object *)object;

	/* Release our reference to this Python object. */
	Py_XDECREF(pip->object);
}
/* }}} */
/* {{{ python_object_free(void *object TSRMLS_DC)
 */
static void
python_object_free(void *object TSRMLS_DC)
{
	php_python_object *pip = (php_python_object *)object;

	/* Free the object's resources using the PHP allocator. */
	efree(pip);
}
/* }}} */
/* {{{ python_object_clone(void *object, void **clone_ptr TSRMLS_DC)
 */
static void
python_object_clone(void *object, void **clone_ptr TSRMLS_DC)
{
	php_python_object *orig, *clone;

	orig = (php_python_object *)object;
	clone = (php_python_object *)emalloc(sizeof(php_python_object));

	/*
	 * XXX: Should we call __copy__ on the original Python object and store the
	 * result in the clone instead of just memcpy'ing the entire structure?
	 */

	memcpy(clone, orig, sizeof(php_python_object));

	/* Add a new reference to the cloned (copied) Python object. */
	Py_INCREF(clone->object);

	*clone_ptr = clone;
}
/* }}} */
/* {{{ python_object_create(zend_class_entry *ce TSRMLS_DC)
 */
zend_object_value
python_object_create(zend_class_entry *ce TSRMLS_DC)
{
	php_python_object *pip;
	zend_object_value retval;

	/* Allocate and initialize the PHP Python object structure. */
	pip = emalloc(sizeof(php_python_object));
	pip->object = NULL;
	pip->ce = ce;

	/* Add this instance to the objects store using the Zend Objects API. */
	retval.handle = zend_objects_store_put(pip,
										   python_object_destroy,
										   python_object_free,
										   python_object_clone TSRMLS_CC);
    retval.handlers = &python_object_handlers;

	/* Return the object reference to the caller. */
	return retval;
}
/* }}} */

/* {{{ python_num_args(PyObject *callable)
   Returns the number of arguments expected by the given callable object. */
zend_uint
python_num_args(PyObject *callable)
{
	PyObject *func_code, *co_argcount;
	zend_uint num_args = 0;

	if (func_code = PyObject_GetAttrString(callable, "func_code")) {
		if (co_argcount = PyObject_GetAttrString(func_code, "co_argcount")) {
			num_args = PyInt_AsLong(co_argcount);
			Py_DECREF(co_argcount);
		}
		Py_DECREF(func_code);
	}

	return num_args;
}
/* }}} */
/* {{{ python_get_arg_info(PyObject *callable, zend_arg_info *arg_info)
   Fills out the arg_info array and returns the total number of arguments. */
zend_uint
python_get_arg_info(PyObject *callable, zend_arg_info **arg_info)
{
	PyObject *func_code, *co_varnames;
	zend_uint num_args = 0;

	/* Make sure that we've been passed a valid, callable object. */
	if (!callable || PyCallable_Check(callable) == 0)
		return 0;

	/*
	 * The arguments are described by the object's func_code.co_varnames
	 * member.  They're represented as a Python tuple.
	 */
	if (func_code = PyObject_GetAttrString(callable, "func_code")) {
		if (co_varnames = PyObject_GetAttrString(func_code, "co_varnames")) {
			PyObject *arg;
			int i, num_vars, start = 0;

			/*
			 * Get the number of arguments defined by the co_varnames tuple
			 * and use that value to resize the arg_info array.
			 */
			num_vars = num_args = PyTuple_Size(co_varnames);

			/* If this is a method, skip the explicit "self" argument. */
			if (PyMethod_Check(callable)) {
				start = 1;
				num_args -= 1;
			}

			/* Resize the arg_info array based on the number of arguments. */
			*arg_info = ecalloc(num_args, sizeof(zend_arg_info));

			/* Describe each of this method's arguments. */
			for (i = start; i < num_vars; ++i) {
				arg = PyTuple_GetItem(co_varnames, i);

				/* Fill out the zend_arg_info structure for this argument. */
				if (arg && PyString_Check(arg)) {
					arg_info[i-start]->name = estrdup(PyString_AS_STRING(arg));
					arg_info[i-start]->name_len = PyString_GET_SIZE(arg);
					arg_info[i-start]->class_name = '\0';
					arg_info[i-start]->class_name_len = 0;
					arg_info[i-start]->allow_null = 1;
					arg_info[i-start]->pass_by_reference = 0;
				}
			}

			Py_DECREF(co_varnames);
		}
		Py_DECREF(func_code);
	}

	return num_args;
}
/* }}} */


/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
