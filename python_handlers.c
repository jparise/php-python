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
#include "Zend/zend_exceptions.h"

/* {{{ efree_function(zend_internal_function *func)
   Frees the memory allocated to a zend_internal_function structure. */
inline static void
efree_function(zend_internal_function *func)
{
	int i;

	/* Free the estrdup'ed function name. */
	efree(func->function_name);

	/* Free the argument information. */
	for (i = 0; i < func->num_args; i++) {
		if (func->arg_info[i].name) {
			efree(func->arg_info[i].name);
		}
		if (func->arg_info[i].class_name) {
			efree(func->arg_info[i].class_name);
		}
	}
	efree(func->arg_info);

	/* Free the function structure itself. */
	efree(func);
}
/* }}} */

/* {{{ python_property_read(zval *object, zval *member, zend_bool silent TSRMLS_DC)
 */
static zval *
python_property_read(zval *object, zval *member, zend_bool silent TSRMLS_DC)
{
	php_python_object *obj;
	zval *return_value;

	MAKE_STD_ZVAL(return_value);
	ZVAL_NULL(return_value);

	obj = PY_FETCH(object);

	if (obj && obj->object) {
		PyObject *attr = NULL;

		convert_to_string_ex(&member);

		if (attr = PyObject_GetAttrString(obj->object, Z_STRVAL_P(member))) {
			return_value = pip_pyobject_to_zval(attr);
			Py_DECREF(attr);
		}
	} else if (!silent) {
		/* XXX: "Not a Python object" */
	}

	return return_value;
}
/* }}} */
/* {{{ python_property_write(zval *object, zval *member, zval *value TSRMLS_DC)
 */
static void
python_property_write(zval *object, zval *member, zval *value TSRMLS_DC)
{
	php_python_object *obj;
	PyObject *val = NULL;

	obj = PY_FETCH(object);
	val = pip_zval_to_pyobject(&value);

	if (obj && obj->object && val) {
		convert_to_string_ex(&member);
		PyObject_SetAttrString(obj->object, Z_STRVAL_P(member), val);
	} else {
		/* XXX: "Not a Python object" */
	}
}
/* }}} */
/* {{{ python_read_dimension(zval *object, zval *offset TSRMLS_DC)
 */
static zval *
python_read_dimension(zval *object, zval *offset TSRMLS_DC)
{
	php_python_object *obj;
	zval *return_value;

	MAKE_STD_ZVAL(return_value);
	ZVAL_NULL(return_value);

	obj = PY_FETCH(object);

	if (obj && obj->object) {
		if (PySequence_Check(obj->object) == 1) {
			PyObject *item = NULL;
			
			convert_to_long(offset);

			/* Bounds check */
			if (PySequence_Size(obj->object) <= Z_LVAL_P(offset)) {
				/* XXX: "Index is out of bounds" */
				return return_value;
			}

			item = PySequence_GetItem(obj->object, Z_LVAL_P(offset));
			if (item) {
				return_value = pip_pyobject_to_zval(item);
				Py_DECREF(item);
			}
		} else {
			/* XXX: "Not a sequence type" */
		}
	} else {
		/* XXX: "Not a Python object" */
	}

	return return_value;
}
/* }}} */
/* {{{ python_write_dimension(zval *object, zval *offset, zval *value TSRMLS_DC)
 */
static void
python_write_dimension(zval *object, zval *offset, zval *value TSRMLS_DC)
{
	php_python_object *obj;
	PyObject *val = NULL;

	obj = PY_FETCH(object);
	val = pip_zval_to_pyobject(&value);

	if (obj && obj->object && val) {
		if (PySequence_Check(obj->object) == 1) {
			if (PySequence_SetItem(obj->object, Z_LVAL_P(offset), val) == -1) {
				/* XXX: "Failed" */
			}
		} else {
			/* XXX: "Not a sequence type" */
		}
	} else {
		/* XXX: "Not a Python object" */
	}

	if (val) {
		Py_DECREF(val);
	}
}
/* }}} */
/* {{{ python_object_set(zval **property, zval *value TSRMLS_DC)
 */
static void
python_object_set(zval **property, zval *value TSRMLS_DC)
{
    /* Not yet implemented in the engine */
}
/* }}} */
/* {{{ python_object_get(zval *property TSRMLS_DC)
 */
static zval *
python_object_get(zval *property TSRMLS_DC)
{
    /* Not yet implemented in the engine */
    return NULL;
}
/* }}} */
/* {{{ python_property_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
 */
static int
python_property_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	php_python_object *obj;

	obj = PY_FETCH(object);

	if (obj && obj->object) {
		convert_to_string_ex(&member);
		return PyObject_HasAttrString(obj->object, Z_STRVAL_P(member)) ? 1 : 0;
	}

	return 0;
}
/* }}} */
/* {{{ python_dimension_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
 */
static int
python_dimension_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	php_python_object *obj;
	PyObject *attr = NULL;
	int ret = 0;
   
	obj = PY_FETCH(object);

	convert_to_string_ex(&member);

	if (attr = PyObject_GetAttrString(obj->object, Z_STRVAL_P(member))) {
		ret = PySequence_Check(attr);
		Py_DECREF(attr);
	}

	return ret;
}
/* }}} */
/* {{{ python_property_delete(zval *object, zval *member TSRMLS_DC)
 */
static void
python_property_delete(zval *object, zval *member TSRMLS_DC)
{
	php_python_object *obj;

	obj = PY_FETCH(object);

	if (obj && obj->object) {
		convert_to_string_ex(&member);
		PyObject_DelAttrString(obj->object, Z_STRVAL_P(member));
	} else {
		/* XXX: "Not a Python object" */
	}
}
/* }}} */
/* {{{ python_dimension_delete(zval *object, zval *offset TSRMLS_DC)
 */
static void
python_dimension_delete(zval *object, zval *offset TSRMLS_DC)
{
	php_python_object *obj = PY_FETCH(object);

	if (obj && obj->object) {
		if (PySequence_Check(obj->object) == 1) {
			if (PySequence_DelItem(obj->object, Z_LVAL_P(offset)) == -1) {
				/* XXX: "Failed" */
			}
		} else {
			/* XXX: "Not a sequence type" */
		}
	} else {
		/* XXX: "Not a Python object" */
	}
}
/* }}} */
/* {{{ python_properties_get(zval *object TSRMLS_DC)
 */
static HashTable *
python_properties_get(zval *object TSRMLS_DC)
{
	/* TODO: Return the object's __dict__ */
    return NULL;
}
/* }}} */
/* {{{ python_method_get(zval *object, char *name, int len TSRMLS_DC)
 */
static union _zend_function *
python_method_get(zval *object, char *name, int len TSRMLS_DC)
{
	php_python_object *obj;
	zend_internal_function *f = NULL;
	PyObject *method = NULL;

	obj = PY_FETCH(object);

	/* Quickly check if this object has a method with the requested name. */
	if (!obj || PyObject_HasAttrString(obj->object, name) == 0) {
		return NULL;
	}

	/* Attempt to fetch the method and verify that it's callable. */
	method = PyObject_GetAttrString(obj->object, name);
	if (!method || !PyMethod_Check(method) || PyCallable_Check(method) == 0) {
		if (method) Py_DECREF(method);
		return NULL;
	}

	f = emalloc(sizeof(zend_internal_function));
	f->type = ZEND_OVERLOADED_FUNCTION_TEMPORARY;
	f->function_name = estrndup(name, len);
	f->scope = obj->ce;
	f->fn_flags = 0;
	f->prototype = NULL;
	f->pass_rest_by_reference = 0;
	f->num_args = python_get_arg_info(method, &(f->arg_info));

	Py_DECREF(method);

	return (union _zend_function *)f;
}
/* }}} */
/* {{{ python_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS)
 */
static int
python_call_method(char *method_name, INTERNAL_FUNCTION_PARAMETERS)
{
	php_python_object *obj;
	PyObject *method = NULL;
	zend_internal_function *func;
	int ret = FAILURE;
	int argc = ZEND_NUM_ARGS();
   
	/* Dereference ourself to acquire a php_python_object pointer. */
	obj = PY_FETCH(getThis());

	/* Get a pointer to the requested method from this object. */
	method = PyObject_GetAttrString(obj->object, method_name);

	/* If the class exists and is callable ... */
	if (method && PyCallable_Check(method)) {
		PyObject *args, *result = NULL;

		/* Convert all of our PHP arguments into a Python-digestable tuple. */
		args = pip_args_to_tuple(ZEND_NUM_ARGS(), 0 TSRMLS_CC);

		/*
		 * Invoke the requested method and store the resul. If we have a tuple
		 * of arguments, remember to free (decref) it.
		 */
		result = PyObject_CallObject(method, args); 
		Py_DECREF(method);
		if (args) {
			Py_DECREF(args);
		}

		if (result) {
			/* Convert the Python result to its PHP equivalent */
			*return_value = *pip_pyobject_to_zval(result);
			zval_copy_ctor(return_value);
			Py_DECREF(result);

			ret = SUCCESS;
		}
	}

	/* Reclaim the memory that we allocated for this function in method_get. */
	func = (zend_internal_function *)EG(function_state_ptr)->function;
	efree_function(func);

	return ret;
}
/* }}} */
/* {{{ python_constructor_get(zval *object TSRMLS_DC)
 */
static union _zend_function *
python_constructor_get(zval *object TSRMLS_DC)
{
	php_python_object *obj = PY_FETCH(object);

	return obj->ce->constructor;
}
/* }}} */
/* {{{ python_class_entry_get(zval *object TSRMLS_DC)
 */
static zend_class_entry *
python_class_entry_get(zval *object TSRMLS_DC)
{
	php_python_object *obj = PY_FETCH(object);
	assert(obj);

	return obj->ce;
}
/* }}} */
/* {{{ python_class_name_get(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC)
 */
static int
python_class_name_get(zval *object, char **class_name,
					  zend_uint *class_name_len, int parent TSRMLS_DC)
{
	php_python_object *obj = PY_FETCH(object);

    *class_name = estrndup(obj->ce->name, obj->ce->name_length);
    *class_name_len = obj->ce->name_length;

    return 0;
}
/* }}} */
/* {{{ python_objects_compare(zval *object1, zval *object2 TSRMLS_DC)
 */
static int
python_objects_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	php_python_object *a, *b;

	a = PY_FETCH(object1);
	b = PY_FETCH(object2);

	return PyObject_Compare(a->object, b->object);
}
/* }}} */
/* {{{ python_object_cast(zval *readobj, zval *writeobj, int type, int should_free TSRMLS_DC)
 */
static int
python_object_cast(zval *readobj, zval *writeobj, int type,
				   int should_free TSRMLS_DC)
{
	php_python_object *obj;
	PyObject *val = NULL;
	
	if (should_free) {
		zval_dtor(writeobj);
	}

	ZVAL_NULL(writeobj);

	obj = PY_FETCH(readobj);

	switch (type) {
		case IS_LONG:
		case IS_DOUBLE:
		case IS_BOOL:
			/* TODO */
			break;
		case IS_STRING:
			val = PyObject_Str(obj->object);
			break;
	}

	if (val) {
		writeobj = pip_pyobject_to_zval(val);
		Py_DECREF(val);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ python_object_handlers[]
 */
zend_object_handlers python_object_handlers = {
	ZEND_OBJECTS_STORE_HANDLERS,
    python_property_read,
    python_property_write,
    python_read_dimension,
    python_write_dimension,
    NULL,
    python_object_get,
    python_object_set,
    python_property_exists,
    python_property_delete,
    python_dimension_exists,
    python_dimension_delete,
    python_properties_get,
    python_method_get,
    python_call_method,
    python_constructor_get,
    python_class_entry_get,
    python_class_name_get,
    python_objects_compare,
    python_object_cast
};
/* }}} */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
