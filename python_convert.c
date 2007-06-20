/*
 * Python in PHP - Embedded Python Extension
 *
 * Copyright (c) 2003,2004,2005,2006 Jon Parise <jon@php.net>
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

#include "php.h"
#include "php_python_internal.h"

extern zend_class_entry python_class_entry;

/* PHP to Python Conversions */

/* {{{ pip_hash_to_list(zval *hash TSRMLS_DC)
   Convert a PHP hash to a Python list. */
PyObject *
pip_hash_to_list(zval *hash TSRMLS_DC)
{
	PyObject *list;
	zval **entry;
	int pos = 0;

	/* Make sure we were given a PHP hash. */
	if (Z_TYPE_P(hash) != IS_ARRAY) {
		return NULL;
	}

	/* Create a list with the same number of elements as the hash. */
	list = PyList_New(zend_hash_num_elements(Z_ARRVAL_P(hash)));

	/* Let's start at the very beginning, a very good place to start. */
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(hash));

	/*
	 * Iterate over of the hash's elements.  We ignore the keys and convert
	 * each value to its Python equivalent before inserting it into the list.
	 */
	while (zend_hash_get_current_data(Z_ARRVAL_P(hash),
									  (void **)&entry) == SUCCESS) {
		PyObject *item = pip_zval_to_pyobject(*entry TSRMLS_CC);
		PyList_SetItem(list, pos++, item);
		zend_hash_move_forward(Z_ARRVAL_P(hash));
	}

	return list;
}
/* }}} */
/* {{{ pip_hash_to_tuple(zval *hash TSRMLS_DC)
   Convert a PHP hash to a Python tuple. */
PyObject *
pip_hash_to_tuple(zval *hash TSRMLS_DC)
{
	return PyList_AsTuple(pip_hash_to_list(hash TSRMLS_CC));
}
/* }}} */
/* {{{ pip_hash_to_dict(zval *hash TSRMLS_DC)
   Convert a PHP hash to a Python dictionary. */
PyObject *
pip_hash_to_dict(zval *hash TSRMLS_DC)
{
	PyObject *dict, *integer;
	zval **entry;
	char *string_key;
	long num_key;

	/* Make sure we were given a PHP hash. */
	if (Z_TYPE_P(hash) != IS_ARRAY) {
		return NULL;
	}

	/* Create a new empty dictionary. */
	dict = PyDict_New();

	/* Let's start at the very beginning, a very good place to start. */
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(hash));

	/* Iterate over the hash's elements. */
	while (zend_hash_get_current_data(Z_ARRVAL_P(hash),
									  (void **)&entry) == SUCCESS) {

		/* Convert the PHP value to its Python equivalent (recursion). */
		PyObject *item = pip_zval_to_pyobject(*entry TSRMLS_CC);

		/* Assign the item with the appropriate key type (string or integer). */
		switch (zend_hash_get_current_key(Z_ARRVAL_P(hash), &string_key,
										  &num_key, 0)) {
			case HASH_KEY_IS_STRING:
				PyDict_SetItemString(dict, string_key, item);
				break;
			case HASH_KEY_IS_LONG:
				integer = PyInt_FromLong(num_key);
				PyDict_SetItem(dict, integer, item);
				Py_DECREF(integer);
				break;
		}

		/* Advance to the next entry. */
		zend_hash_move_forward(Z_ARRVAL_P(hash));
	}

	return dict;
}
/* }}} */
/* {{{ pip_zobject_to_pyobject(zval *obj TSRMLS_DC)
   Convert a PHP (Zend) object to a Python object. */
PyObject *
pip_zobject_to_pyobject(zval *obj TSRMLS_DC)
{
	PyObject *dict, *str;
	zval **entry;
	char *string_key;
	long num_key;

	/*
	 * At this point, we represent a PHP object as a dictionary of
	 * its properties.  In the future, we may provide a true object
	 * conversion (which is entirely possible, but it's more work
	 * that I plan on doing right now).
	 */
	dict = PyDict_New();

	/* Start at the beginning of the object properties hash */
	zend_hash_internal_pointer_reset(Z_OBJPROP_P(obj));

	/* Iterate over the hash's elements */
	while (zend_hash_get_current_data(Z_OBJPROP_P(obj),
									  (void **)&entry) == SUCCESS) {

		/* Convert the PHP value to its Python equivalent (recursion) */
		PyObject *item = pip_zval_to_pyobject(*entry TSRMLS_CC);

		switch (zend_hash_get_current_key(Z_OBJPROP_P(obj),
										  &string_key, &num_key, 0)) {
			case HASH_KEY_IS_STRING:
				PyDict_SetItemString(dict, string_key, item);
				break;
			case HASH_KEY_IS_LONG:
				str = PyString_FromFormat("%d", num_key);
				PyObject_SetItem(dict, str, item);
				Py_DECREF(str);
				break;
			case HASH_KEY_NON_EXISTANT:
				php_error(E_ERROR, "Hash key is nonexistent");
				break;
		}

		/* Advance to the next entry */
		zend_hash_move_forward(Z_OBJPROP_P(obj));
	}

	return dict;
}
/* }}} */
/* {{{ pip_zval_to_pyobject(zval *val TSRMLS_DC)
   Converts the given zval into an equivalent PyObject. */
PyObject *
pip_zval_to_pyobject(zval *val TSRMLS_DC)
{
	PyObject *ret;

	if (val == NULL) {
		return NULL;
	}

	switch (Z_TYPE_P(val)) {
	case IS_BOOL:
		ret = PyBool_FromLong(Z_LVAL_P(val));
		break;
	case IS_LONG:
		ret = PyLong_FromLong(Z_LVAL_P(val));
		break;
	case IS_DOUBLE:
		ret = PyFloat_FromDouble(Z_DVAL_P(val));
		break;
	case IS_STRING:
		ret = PyString_FromStringAndSize(Z_STRVAL_P(val), Z_STRLEN_P(val));
		break;
#if 0
	case IS_UNICODE:
		ret = PyUnicode_FromUnicode((const UChar *)Z_UNIVAL_P(val),
									Z_UNILEN_P(val));
		break;
#endif
	case IS_ARRAY:
		ret = pip_hash_to_dict(val TSRMLS_CC);
		break;
	case IS_OBJECT:
		ret = pip_zobject_to_pyobject(val TSRMLS_CC);
		break;
	case IS_NULL:
		Py_INCREF(Py_None);
		ret = Py_None;
		break;
	default:
		ret = NULL;
		break;
	}

	return ret;
}
/* }}} */

/* Python to PHP Conversions */

/* {{{ pip_sequence_to_hash(PyObject *seq, HashTable *ht TSRMLS_DC)
   Convert a Python sequence to a PHP hash. */
int
pip_sequence_to_hash(PyObject *seq, HashTable *ht TSRMLS_DC)
{
	int i;

	/* Make sure this object implements the sequence protocol. */
	if (!PySequence_Check(seq)) {
		return FAILURE;
	}

	for (i = 0; i < PySequence_Size(seq); ++i) {
		/* Get the item in this slot and convert it to a zval. */
		PyObject *item = PySequence_GetItem(seq, i);
		zval *val = pip_pyobject_to_zval(item TSRMLS_CC);
		Py_XDECREF(item);

		/* Append the zval to our hash. */
		if (zend_hash_next_index_insert(ht, (void *)&val, sizeof(zval *),
										NULL) == FAILURE) {
			php_error(E_ERROR, "Python: Hash insertion failure");
		}
	}

	return SUCCESS;
}
/* }}} */
/* {{{ pip_sequence_to_array(PyObject *seq TSRMLS_DC)
   Convert a Python sequence to a PHP array. */
zval *
pip_sequence_to_array(PyObject *seq TSRMLS_DC)
{
	zval *array;

	/* Initialize our PHP array. */
	MAKE_STD_ZVAL(array);
	if (array_init(array) != SUCCESS) {
		return NULL;
	}

	/* Convert the sequence and place the result in the array's hashtable. */
	if (pip_sequence_to_hash(seq, Z_ARRVAL_P(array) TSRMLS_CC) != SUCCESS) {
		return NULL;
	}

	return array;
}
/* }}} */
/* {{{ pip_mapping_to_hash(PyObject *map, HashTable *ht TSRMLS_DC)
   Convert a Python mapping to a PHP hash. */
int
pip_mapping_to_hash(PyObject *map, HashTable *ht TSRMLS_DC)
{
	PyObject *keys, *key;
	char *key_name;
	int i, key_len;

	/* Retrieve the list of keys for this mapping. */
	if ((keys = PyMapping_Keys(map)) == NULL) {
		return FAILURE;
	}

	/* Iterate over the list of keys. */
	for (i = 0; i < PySequence_Size(keys); ++i) {
		if (key = PySequence_GetItem(keys, i)) {
			PyObject *str, *item;

			/* Get the string representation of this key. */
			str = PyObject_Str(key);
			PyString_AsStringAndSize(str, &key_name, &key_len);
			Py_XDECREF(str);

			/* Extract the item for this key. */
			if (item = PyMapping_GetItemString(map, key_name)) {
				zval *val = pip_pyobject_to_zval(item TSRMLS_CC);

				/* Add the new item to the hash. */
				if (zend_hash_update(ht, key_name, key_len + 1,
									 (void *)&val, sizeof(zval *),
									 NULL) == FAILURE) {
					php_error(E_ERROR, "Python: hash update failure");
				}
				Py_DECREF(item);
			}
			Py_DECREF(key);
		}
	}

	Py_DECREF(keys);
	return SUCCESS;
}
/* }}} */
/* {{{ pip_mapping_to_array(PyObject *map TSRMLS_DC)
   Convert a Python mapping to a PHP array. */
zval *
pip_mapping_to_array(PyObject *map TSRMLS_DC)
{
	zval *array;

	/* Initialize our PHP array. */
	MAKE_STD_ZVAL(array);
	if (array_init(array) != SUCCESS) {
		return NULL;
	}

	/* Convert the sequence and place the result in the array's hashtable. */
	if (pip_mapping_to_hash(map, Z_ARRVAL_P(array) TSRMLS_CC) != SUCCESS) {
		return NULL;
	}

	return array;
}
/* }}} */
/* {{{ pip_pyobject_to_zobject(PyObject *obj TSRMLS_DC)
   Convert Python object to a PHP (Zend) object */
zval *
pip_pyobject_to_zobject(PyObject *obj TSRMLS_DC)
{
	php_python_object *pip;
	zval *ret;

	/* Create a new instance of a PHP Python object. */
	MAKE_STD_ZVAL(ret);
	if (object_init_ex(ret, &python_class_entry) != SUCCESS) {
		return ret;
	}

	/*
	 * Fetch the php_python_object data for this object instance. Bump the
	 * reference count of our Python object and associate it with our PHP
	 * Python object instance.
	 */
	pip = (php_python_object *)zend_object_store_get_object(ret TSRMLS_CC);
	Py_INCREF(obj);
	pip->object = obj;

	return ret;
}
/* }}} */
/* {{{ pip_pyobject_to_zval(PyObject *obj TSRMLS_DC)
   Converts the given PyObject into an equivalent zval. */
zval *
pip_pyobject_to_zval(PyObject *obj TSRMLS_DC)
{
	zval *ret = NULL;

	if (obj == NULL || obj == Py_None) {
		MAKE_STD_ZVAL(ret);
		ZVAL_NULL(ret);
	} else if (PyInt_Check(obj)) {
		MAKE_STD_ZVAL(ret);
		ZVAL_LONG(ret, PyInt_AsLong(obj));
	} else if (PyLong_Check(obj)) {
		MAKE_STD_ZVAL(ret);
		ZVAL_LONG(ret, PyLong_AsLong(obj));
	} else if (PyFloat_Check(obj)) {
		MAKE_STD_ZVAL(ret);
		ZVAL_DOUBLE(ret, PyFloat_AsDouble(obj));
	} else if (PyString_Check(obj)) {
		MAKE_STD_ZVAL(ret);
		ZVAL_STRINGL(ret, PyString_AsString(obj), PyString_Size(obj), 1);
	} else if (PySequence_Check(obj)) {
		ret = pip_sequence_to_array(obj TSRMLS_CC);
	} else if (PyMapping_Check(obj)) {
		ret = pip_mapping_to_array(obj TSRMLS_CC);
	} else {
		ret = pip_pyobject_to_zobject(obj TSRMLS_CC);
	}

	/* If we still don't have a valid value, return (PHP's) NULL. */
	if (ret == NULL) {
		MAKE_STD_ZVAL(ret);
		ZVAL_NULL(ret);
	}

	return ret;
}
/* }}} */

/* Argument Conversions */

/* {{{ pip_args_to_tuple(int argc, int start TSRMLS_DC)
   Converts PHP arguments into a Python tuple suitable for argument passing. */
PyObject *
pip_args_to_tuple(int argc, int start TSRMLS_DC)
{
	PyObject *args = NULL;
	zval ***zargs;

	if (argc < start) {
		return NULL;
	}

	/* Allocate enough memory for all of the arguments. */
	if ((zargs = (zval ***) emalloc(sizeof(zval **) * argc)) == NULL) {
		return NULL;
	}

	/* Get the array of PHP parameters. */
	if (zend_get_parameters_array_ex(argc, zargs) == SUCCESS) {
		int i = 0;

		args = PyTuple_New(argc - start);
		for (i = start; i < argc; i++) {
			PyObject *arg = pip_zval_to_pyobject(*zargs[i] TSRMLS_CC);
			PyTuple_SetItem(args, i - start, arg);
		}
	}

	/* We're done with the PHP parameter array.  Free it. */
	efree(zargs);

	return args;
}
/* }}} */
/* {{{ pip_args_to_tuple_ex(int ht, int argc, int start TSRMLS_DC)
   Converts PHP arguments into a Python tuple suitable for argument passing */
PyObject *
pip_args_to_tuple_ex(int ht, int argc, int start TSRMLS_DC)
{
	PyObject *args = NULL;
	zval **zargs;

	if (argc < start) {
		return NULL;
	}

	/* Allocate enough memory for all of the arguments. */
	if ((zargs = (zval **) emalloc(sizeof(zval *) * argc)) == NULL) {
		return NULL;
	}

	/* Get the array of PHP parameters. */
	if (zend_get_parameters_array(ht, argc, zargs) == SUCCESS) {
		int i = 0;

		args = PyTuple_New(argc - start);
		for (i = start; i < argc; i++) {
			PyObject *arg = pip_zval_to_pyobject(zargs[i] TSRMLS_CC);
			PyTuple_SetItem(args, i - start, arg);
		}
	}

	/* We're done with the PHP parameter array.  Free it. */
	efree(zargs);

	return args;
}
/* }}} */

/* Object Representations */

/* {{{ pip_str(PyObject *obj, char **buffer, int *length)
   Returns the NUL-terminated string representation of the Python object. */
int
pip_str(PyObject *obj, char **buffer, int *length)
{
	PyObject *str = PyObject_Str(obj);
	int ret = -1;

	if (str) {
		ret = PyString_AsStringAndSize(str, buffer, length);
		Py_DECREF(str);
	}

	return ret;
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
