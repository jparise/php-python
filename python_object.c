/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Jon Parise  <jon@php.net>                                    |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "php.h"
#include "php_python_internal.h"

/* {{{ python_object_create(zend_class_entry *ce TSRMLS_DC)
 */
zend_object_value
python_object_create(zend_class_entry *ce TSRMLS_DC)
{
	php_python_object *obj;
	zend_object_value retval;

	obj = emalloc(sizeof(php_python_object));
	memset(obj, 0, sizeof(php_python_object));

	obj->ce = ce;

	retval.handle = zend_objects_store_put(obj, python_object_destroy,
										   python_object_clone TSRMLS_CC);
    retval.handlers = &python_object_handlers;

	return retval;
}
/* }}} */
/* {{{ python_object_destroy(void *object, zend_object_handle handle TSRMLS_DC)
 */
void
python_object_destroy(void *object, zend_object_handle handle TSRMLS_DC)
{
	php_python_object *obj = (php_python_object *)object;

	/* Release our reference to this Python object. */
	if (obj->object) {
		Py_DECREF(obj->object);
	}

	/* Free the object's resources using the PHP allocator. */
	efree(obj);
}
/* }}} */
/* {{{ python_object_clone(void *object, void **clone_ptr TSRMLS_DC)
 */
void
python_object_clone(void *object, void **clone_ptr TSRMLS_DC)
{
	php_python_object *orig, *clone;
	
	fprintf(stderr, "python_object_clone\n");
	
	orig = (php_python_object *)object;
	clone = (php_python_object *)emalloc(sizeof(php_python_object));

	/*
	 * XXX: Should we call __copy__ on the original Python object and store the
	 * resulting in the clone instead of just memcpy'ing the entire structure?
	 */

	memcpy(clone, orig, sizeof(php_python_object));

	/* Add a new reference to the cloned (copied) Python object. */
	Py_INCREF(clone->object);

	*clone_ptr = clone;
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
	if (!callable || PyCallable_Check(callable) == 0) {
		return 0;
	}

	/*
	 * The arguments are described by the object's func_code.co_varnames
	 * member.  They're represented as a Python tuple.
	 */
	if (func_code = PyObject_GetAttrString(callable, "func_code")) {
		if (co_varnames = PyObject_GetAttrString(func_code, "co_varnames")) {
			PyObject *arg;
			int start = 0, i;

			/*
			 * Get the number of arguments defined by the co_varnames tuple
			 * and use that value to resize the arg_info array.
			 */
			num_args = PyTuple_Size(co_varnames);

			/* If this is a method, skip the explicit "self" argument. */
			if (PyMethod_Check(callable)) {
				start = 1;
				num_args -= 1;
			}

			/* Resize the arg_info array based on the number of arguments. */
			*arg_info = ecalloc(num_args, sizeof(zend_arg_info));

			/* Describe each of this method's arguments. */
			for (i = start; i < PyTuple_Size(co_varnames); i++) {
				arg = PyTuple_GetItem(co_varnames, i);

				/* Fill out the zend_arg_info structure for this argument. */
				if (arg && PyString_Check(arg)) {
					arg_info[i - start]->name = estrdup(PyString_AsString(arg));
					arg_info[i - start]->name_len = PyString_Size(arg);
					arg_info[i - start]->class_name = '\0';
					arg_info[i - start]->class_name_len = 0;
					arg_info[i - start]->allow_null = 1;
					arg_info[i - start]->pass_by_reference = 0;
				}
			}

			Py_DECREF(co_varnames);
		}
		Py_DECREF(func_code);
	}

	return num_args;
}
/* }}} */

/* {{{ PHP_FUNCTION(python_new)
 */
PHP_FUNCTION(python_new)
{
	php_python_object *obj;
	PyObject *module;
	char *module_name, *class_name;
	int module_name_len, class_name_len;

	/* Dereference ourself to acquire a php_python_object pointer. */
	obj = PIP_FETCH(getThis());

	/*
	 * We expect at least two parameters: the module name and the class name.
	 * Any additional parameters will be passed to the Python __init__ method
	 * down below.
	 */
	if (zend_parse_parameters(2 TSRMLS_CC, "ss",
							  &module_name, &module_name_len,
							  &class_name, &class_name_len) == FAILURE) {
		printf("Woops\n");
		return;
	}

	module = PyImport_ImportModule(module_name);
	if (module) {
		PyObject *dict, *class;

		/*
		 * The module's dictionary holds references to all of its members.  Use
		 * it to acquire a pointer to the requested class.
		 */
		dict = PyModule_GetDict(module);
		class = PyDict_GetItemString(dict, class_name);

		/* If the class exists and is callable ... */
		if (class && PyCallable_Check(class)) {
			PyObject *args = NULL;

			/*
			 * Convert our PHP arguments into a Python-digestable tuple.  We
			 * skip the first two arguments (module name, class name) and pass
			 * the rest to the Python constructor.
			 */
			args = pip_args_to_tuple(ZEND_NUM_ARGS(), 2 TSRMLS_CC);

			/*
			 * Call the class's constructor and store the resulting object.  If
			 * we have a tuple of arguments, remember to free (decref) it.
			 */
			obj->object = PyObject_CallObject(class, args);
			if (args) {
				Py_DECREF(args);
			}

			/* Our new object should be an instance of the requested class. */
			assert(PyObject_IsInstance(obj->object, class) == 1);
		}
	}
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
