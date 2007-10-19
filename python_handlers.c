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

#include "php.h"
#include "php_python_internal.h"

/* Helpers */
/* {{{ efree_function(zend_internal_function *func)
   Frees the memory allocated to a zend_internal_function structure. */
static void
efree_function(zend_internal_function *func)
{
	int i;

	/* Free the estrdup'ed function name. */
	efree(func->function_name);

	/* Free the argument information. */
	for (i = 0; i < func->num_args; ++i) {
		if (func->arg_info[i].name)
			efree(func->arg_info[i].name);

		if (func->arg_info[i].class_name)
			efree(func->arg_info[i].class_name);
	}
	efree(func->arg_info);

	/* Free the function structure itself. */
	efree(func);
}
/* }}} */
/* {{{ merge_class_dict(PyObject *o, HashTable *ht TSRMLS_DC)
   Merge the contents of the class's __dict__ attribute into the hashtable. */
static int
merge_class_dict(PyObject *o, HashTable *ht TSRMLS_DC)
{
	PyObject *d;
	PyObject *bases;

	/* We assume that the Python object is a class type. */
	assert(PyClass_Check(o));

	/*
	 * Start by attempting to merge the contents of the class type's __dict__.
	 * It's alright if the class type doesn't have a __dict__ attribute.
	 */
	d = PyObject_GetAttrString(o, "__dict__");
	if (d == NULL)
		PyErr_Clear();
	else {
		int result = pip_mapping_to_hash(d, ht TSRMLS_CC);
		Py_DECREF(d);
		if (result != SUCCESS)
			return FAILURE;
	}

	/*
	 * We now attempt to recursively merge in the base types' __dicts__s.
	 */
	bases = PyObject_GetAttrString(o, "__bases__");
	if (bases == NULL)
		PyErr_Clear();
	else {
		Py_ssize_t i, n;
		n = PySequence_Size(bases);
		if (n < 0)
			PyErr_Clear();
		else {
			for (i = 0; i < n; i++) {
				int status;
				PyObject *base = PySequence_GetItem(bases, i);
				if (base == NULL) {
					Py_DECREF(bases);
					return FAILURE;
				}

				/* Recurse through this base class. */
				status = merge_class_dict(base, ht TSRMLS_CC);
				Py_DECREF(base);
				if (status != SUCCESS) {
					Py_DECREF(bases);
					return FAILURE;
				}
			}

		}

		Py_DECREF(bases);
	}

	return SUCCESS;
}
/* }}} */

/* Object Handlers */
/* {{{ python_read_property(zval *object, zval *member, int type TSRMLS_DC)
 */
static zval *
python_read_property(zval *object, zval *member, int type TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	zval *return_value = NULL;
	PyObject *attr;

	convert_to_string_ex(&member);

	attr = PyObject_GetAttrString(pip->object, Z_STRVAL_P(member));
	if (attr) {
		MAKE_STD_ZVAL(return_value);
		pip_pyobject_to_zval(attr, return_value TSRMLS_CC);
		Py_DECREF(attr);
	} else
		PyErr_Clear();

	/* TODO: Do something with the 'type' parameter? */

	return return_value;
}
/* }}} */
/* {{{ python_write_property(zval *object, zval *member, zval *value TSRMLS_DC)
 */
static void
python_write_property(zval *object, zval *member, zval *value TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	PyObject *val;
	
	val = pip_zval_to_pyobject(value TSRMLS_CC);
	if (val) {
		convert_to_string_ex(&member);
		if (PyObject_SetAttrString(pip->object, Z_STRVAL_P(member), val) == -1) {
			PyErr_Clear();
			php_error(E_ERROR, "Python: Failed to set attribute %s",
					  Z_STRVAL_P(member));
		}
	}
}
/* }}} */
/* {{{ python_read_dimension(zval *object, zval *offset, int type TSRMLS_DC)
 */
static zval *
python_read_dimension(zval *object, zval *offset, int type TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	zval *return_value = NULL;
	PyObject *item = NULL;

	/*
	 * If we've been given a numeric value, start by attempting to use the
	 * sequence protocol.  The value may be a valid index.
	 */
	if (Z_TYPE_P(offset) == IS_LONG)
		item = PySequence_GetItem(pip->object, Z_LVAL_P(offset));

	/*
	 * Otherwise, if this object provides the mapping protocol, our offset's
	 * string representation may be a key value.
	 */
	if (!item && PyMapping_Check(pip->object)) {
		convert_to_string_ex(&offset);
		item = PyMapping_GetItemString(pip->object, Z_STRVAL_P(offset));
	}

	/* If we successfully fetched an item, return its PHP representation. */
	if (item) {
		MAKE_STD_ZVAL(return_value);
		pip_pyobject_to_zval(item, return_value TSRMLS_CC);
		Py_DECREF(item);
	} else
		PyErr_Clear();

	/* TODO: Do something with the 'type' parameter? */

	return return_value;
}
/* }}} */
/* {{{ python_write_dimension(zval *object, zval *offset, zval *value TSRMLS_DC)
 */
static void
python_write_dimension(zval *object, zval *offset, zval *value TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	PyObject *val = pip_zval_to_pyobject(value TSRMLS_CC);

	/*
	 * If this offset is a numeric value, we'll start by attempting to use
	 * the sequence protocol to set this item.
	 */
	if (Z_TYPE_P(offset) == IS_LONG && PySequence_Check(pip->object)) {
		if (PySequence_SetItem(pip->object, Z_LVAL_P(offset), val) == -1) {
			PyErr_Clear();
			php_error(E_ERROR, "Python: Failed to set sequence item %ld",
					  Z_LVAL_P(offset));
		}
	}

	/*
	 * Otherwise, if this object supports the mapping protocol, use the string
	 * representation of the offset as the key value.
	 */
	else if (PyMapping_Check(pip->object)) {
		convert_to_string_ex(&offset);
		if (PyMapping_SetItemString(pip->object, Z_STRVAL_P(offset), val) == -1) {
			PyErr_Clear();
			php_error(E_ERROR, "Python: Failed to set mapping item '%s'",
					  Z_STRVAL_P(offset));
		}
	}

	Py_XDECREF(val);
}
/* }}} */
/* {{{ python_property_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
 */
static int
python_property_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	PyObject *attr;

	/* We're only concerned with the string representation of this value. */
	convert_to_string_ex(&member);

	/*
	 * If check_empty is specified, we need to check whether or not the
	 * attribute exists *and* is considered "true".  This requires to fetch
	 * and inspect its value.  Like python_dimension_exists() below, we apply
	 * Python's notion of truth here.
	 */
	if (check_empty) {
		attr = PyObject_GetAttrString(pip->object, Z_STRVAL_P(member));
		if (attr) {
			int ret = (PyObject_IsTrue(attr) == 1);
			Py_DECREF(attr);
			return ret;
		} else
			PyErr_Clear();
	}

	/* Otherwise, just check for the existence of the attribute. */
	return PyObject_HasAttrString(pip->object, Z_STRVAL_P(member)) ? 1 : 0;
}
/* }}} */
/* {{{ python_dimension_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
 */
static int
python_dimension_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	PyObject *item = NULL;
	int ret = 0;

	/*
	 * If we've been handed an integer value, check if this is a valid item
	 * index.  PySequence_GetItem() will perform a PySequence_Check() test.
	 */
	if (Z_TYPE_P(member) == IS_LONG)
		item = PySequence_GetItem(pip->object, Z_LVAL_P(member));

	/*
	 * Otherwise, check if this object provides the mapping protocol.  If it
	 * does, check if the string representation of our value is a valid key.
	 */
	if (!item && PyMapping_Check(pip->object)) {
		convert_to_string_ex(&member);
		item = PyMapping_GetItemString(pip->object, Z_STRVAL_P(member));
	}

	/*
	 * If we have a valid item at this point, we have a chance at success.  The
	 * last thing we need to consider is whether or not the item's value is
	 * considered "true" if check_empty has been specified.  We use Python's
	 * notion of truth here for consistency, although it may be more correct to
	 * use PHP's notion of truth (as determined by zend_is_true()) should we
	 * encountered problems with this in the future.
	 */
	if (item) {
		ret = (check_empty) ? (PyObject_IsTrue(item) == 1) : 1;
		Py_DECREF(item);
	} else
		PyErr_Clear();

	return ret;
}
/* }}} */
/* {{{ python_property_delete(zval *object, zval *member TSRMLS_DC)
 */
static void
python_property_delete(zval *object, zval *member TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);

	convert_to_string_ex(&member);
	if (PyObject_DelAttrString(pip->object, Z_STRVAL_P(member)) == -1) {
		PyErr_Clear();
		php_error(E_ERROR, "Python: Failed to delete attribute '%s'",
				  Z_STRVAL_P(member));
	}
}
/* }}} */
/* {{{ python_dimension_delete(zval *object, zval *offset TSRMLS_DC)
 */
static void
python_dimension_delete(zval *object, zval *offset TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	int deleted = 0;

	/*
	 * If we've been given a numeric offset and this object provides the
	 * sequence protocol, attempt to delete the item using a sequence index.
	 */
	if (Z_TYPE_P(offset) == IS_LONG && PySequence_Check(pip->object))
		deleted = PySequence_DelItem(pip->object, Z_LVAL_P(offset)) != -1;

	/*
	 * If we failed to delete the item using the sequence protocol, use the
	 * offset's string representation and try the mapping protocol.
	 */
	if (PyMapping_Check(pip->object)) {
		convert_to_string_ex(&offset);
		deleted = PyMapping_DelItemString(pip->object,Z_STRVAL_P(offset)) != -1;
	}

	/* If we still haven't deleted the requested item, trigger an error. */
	if (!deleted) {
		PyErr_Clear();
		php_error(E_ERROR, "Python: Failed to delete item");
	}
}
/* }}} */
/* {{{ python_get_properties(zval *object TSRMLS_DC)
 */
static HashTable *
python_get_properties(zval *object TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	HashTable *ht;
	PyObject *o;
	int status;

	/*
	 * Allocate a hashtable into which we will copy this Python object's
	 * properties.  If we can't initialize the hashtable, we can't continue.
	 */
	ALLOC_HASHTABLE(ht);
	if (zend_hash_init(ht, 0, NULL, ZVAL_PTR_DTOR, 0) != SUCCESS) {
		FREE_HASHTABLE(ht);
		return NULL;
	}

	/*
	 * If the object supports the sequence or mapping protocol, just copy
	 * the container's contents into the output hashtable.
	 *
	 * XXX: This isn't strictly correct; it's a side-effect of our approach
	 * of treating all Python objects as PHP objects.  We'd really like PHP
	 * to attempt to attempt to "cast" our Python object to an array-like
	 * object first and then use the dimension APIs, in the spirit of
	 * Python's "duck typing", but those casting operations don't currently
	 * exist.  We now have the potential for false-positive conversions
	 * below, where we return the sequence or mapping contents of a Python
	 * object that also has a legitimate set of additional properties.
	 */
	if (PySequence_Check(pip->object)) {
		if (pip_sequence_to_hash(pip->object, ht TSRMLS_CC) != SUCCESS) {
			FREE_HASHTABLE(ht);
			return NULL;
		}
		return ht;
	}
	if (PyMapping_Check(pip->object)) {
		if (pip_mapping_to_hash(pip->object, ht TSRMLS_CC) != SUCCESS) {
			FREE_HASHTABLE(ht);
			return NULL;
		}
		return ht;
	}

	/*
	 * Attempt to append the contents of this object's __dict__ attribute to
	 * our hashtable.  If this object has no __dict__, we have no properties
	 * and return failure.
	 */
	o = PyObject_GetAttrString(pip->object, "__dict__");
	if (o == NULL) {
		PyErr_Clear();
		FREE_HASHTABLE(ht);
		return NULL;
	}

	status = pip_mapping_to_hash(o, ht TSRMLS_CC);
	Py_DECREF(o);
	if (status != SUCCESS) {
		FREE_HASHTABLE(ht);
		return NULL;
	}

	/*
	 * We also attempt to merge any properties inherited from our base
	 * class(es) into the final result.
	 */
	o = PyObject_GetAttrString(pip->object, "__class__");
	if (o) {
		status = merge_class_dict(o, ht TSRMLS_CC);
		Py_DECREF(o);
		if (status != SUCCESS) {
			FREE_HASHTABLE(ht);
			return NULL;
		}
	}

    return ht;
}
/* }}} */
/* {{{ python_get_method(zval **object_ptr, char *method, int method_len TSRMLS_DC)
 */
static union _zend_function *
python_get_method(zval **object_ptr, char *method, int method_len TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, *object_ptr);
	zend_internal_function *f;
	PyObject *func;

	/* Quickly check if this object has a method with the requested name. */
	if (PyObject_HasAttrString(pip->object, method) == 0)
		return NULL;

	/* Attempt to fetch the requested method and verify that it's callable. */
	func = PyObject_GetAttrString(pip->object, method);
	if (!func || PyMethod_Check(func) == 0 || PyCallable_Check(func) == 0) {
		Py_XDECREF(func);
		return NULL;
	}

	/*
	 * Set up the function call structure for this method invokation.  We
	 * allocate a bit of memory here which will be released later on in
	 * python_call_method().
	 */
	f = emalloc(sizeof(zend_internal_function));
	f->type = ZEND_OVERLOADED_FUNCTION_TEMPORARY;
	f->function_name = estrndup(method, method_len);
	f->scope = pip->ce;
	f->fn_flags = 0;
	f->prototype = NULL;
	f->pass_rest_by_reference = 0;
	f->num_args = python_get_arg_info(func, &(f->arg_info));

	Py_DECREF(func);

	return (union _zend_function *)f;
}
/* }}} */
/* {{{ python_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS)
 */
static int
python_call_method(char *method_name, INTERNAL_FUNCTION_PARAMETERS)
{
	PHP_PYTHON_FETCH(pip, getThis());
	PyObject *method;
	int ret = FAILURE;

	/* Get a pointer to the requested method from this object. */
	method = PyObject_GetAttrString(pip->object, method_name);

	/* If the method exists and is callable ... */
	if (method && PyMethod_Check(method) && PyCallable_Check(method)) {
		PyObject *args, *result;

		/* Convert all of our PHP arguments into a Python-digestable tuple. */
		args = pip_args_to_tuple(ZEND_NUM_ARGS(), 0 TSRMLS_CC);

		/*
		 * Invoke the requested method and store the result.  If we have a
		 * tuple of arguments, remember to free (decref) it.
		 */
		result = PyObject_CallObject(method, args); 
		Py_DECREF(method);
		Py_XDECREF(args);

		if (result) {
			ret = pip_pyobject_to_zval(result, return_value TSRMLS_CC);
			Py_DECREF(result);
		}
	}

	/* Release the memory that we allocated for this function in method_get. */
	efree_function((zend_internal_function *)EG(function_state_ptr)->function);

	return ret;
}
/* }}} */
/* {{{ python_constructor_get(zval *object TSRMLS_DC)
 */
static union _zend_function *
python_constructor_get(zval *object TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	return pip->ce->constructor;
}
/* }}} */
/* {{{ python_get_class_entry(zval *object TSRMLS_DC)
 */
static zend_class_entry *
python_get_class_entry(zval *object TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	return pip->ce;
}
/* }}} */
/* {{{ python_get_class_name(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC)
 */
static int
python_get_class_name(zval *object, char **class_name,
					  zend_uint *class_name_len, int parent TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	const char * const key = (parent) ? "__module__" : "__class__";
	PyObject *attr, *str;

	/* Start off with some safe initial values. */
	*class_name = NULL;
	*class_name_len = 0;

	/*
	 * Attempt to use the Python object instance's special read-only attributes
	 * to determine object's class name.  We use __class__ unless we've been
	 * asked for the name of our parent, in which case we use __module__.  We
	 * prefix the class name with "Python " to avoid confusion with native PHP
	 * classes.
	 */
	if (attr = PyObject_GetAttrString(pip->object, key)) {
		if (str = PyObject_Str(attr)) {
			*class_name_len = sizeof("Python ") + PyString_GET_SIZE(str);
			*class_name = (char *)emalloc(sizeof(char *) * *class_name_len);
			zend_sprintf(*class_name, "Python %s", PyString_AS_STRING(str));
			Py_DECREF(str);
		}
		Py_DECREF(attr);
	}

	/* If we still don't have a string, use the PHP class entry's name. */
	if (*class_name_len == 0) {
		*class_name = estrndup(pip->ce->name, pip->ce->name_length);
		*class_name_len = pip->ce->name_length;
	}

	return SUCCESS;
}
/* }}} */
/* {{{ python_compare(zval *object1, zval *object2 TSRMLS_DC)
 */
static int
python_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	PHP_PYTHON_FETCH(a, object1);
	PHP_PYTHON_FETCH(b, object2);

	return PyObject_Compare(a->object, b->object);
}
/* }}} */
/* {{{ python_cast(zval *readobj, zval *writeobj, int type TSRMLS_DC)
 */
static int
python_cast(zval *readobj, zval *writeobj, int type TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, readobj);
	PyObject *val = NULL;
	int ret = FAILURE;

	switch (type) {
		case IS_STRING:
			val = PyObject_Str(pip->object);
			break;
		default:
			return FAILURE;
	}

	if (val) {
		ret = pip_pyobject_to_zval(val, writeobj TSRMLS_CC);
		Py_DECREF(val);
	}

	return ret;
}
/* }}} */
/* {{{ python_count_elements(zval *object, long *count TSRMLS_DC)
   Updates count with the number of elements present in the object and returns
   SUCCESS.  If the object has no sense of overloaded dimensions, FAILURE is
   returned. */
static int
python_count_elements(zval *object, long *count TSRMLS_DC)
{
	PHP_PYTHON_FETCH(pip, object);
	int len = PyObject_Length(pip->object);

	if (len != -1) {
		*count = len;
		return SUCCESS;
	}

	return FAILURE;
}
/* }}} */

/* {{{ python_object_handlers[]
 */
zend_object_handlers python_object_handlers = {
	ZEND_OBJECTS_STORE_HANDLERS,
	python_read_property,
	python_write_property,
	python_read_dimension,
	python_write_dimension,
	NULL,
	NULL,
	NULL,
	python_property_exists,
	python_property_delete,
	python_dimension_exists,
	python_dimension_delete,
	python_get_properties,
	python_get_method,
	python_call_method,
	python_constructor_get,
	python_get_class_entry,
	python_get_class_name,
	python_compare,
	python_cast,
	python_count_elements
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
