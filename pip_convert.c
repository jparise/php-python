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

#include "pip_convert.h"

#include "php.h"

extern int le_pyobject;
extern zend_class_entry python_class_entry;

/* PHP to Python Conversions */

/* {{{ pip_hash_to_list(zval **hash)
   Convert a PHP hash to a Python list */
PyObject *
pip_hash_to_list(zval **hash)
{
	PyObject *list, *item;
	zval **entry;
	long pos = 0;

	/* Make sure we were given a PHP hash */
	if (Z_TYPE_PP(hash) != IS_ARRAY) {
		return NULL;
	}

	/* Create a list with the same number of elements as the hash */
	list = PyList_New(zend_hash_num_elements(Z_ARRVAL_PP(hash)));

	/* Let's start at the very beginning, a very good place to start. */
	zend_hash_internal_pointer_reset(Z_ARRVAL_PP(hash));

	/* Iterate over the hash's elements */
	while (zend_hash_get_current_data(Z_ARRVAL_PP(hash),
									  (void **)&entry) == SUCCESS) {

		/* Convert the PHP value to its Python equivalent */
		item = pip_zval_to_pyobject(entry);

		/* Add this item to the list and increment the position */
		PyList_SetItem(list, pos++, item);

		/* Advance to the next entry */
		zend_hash_move_forward(Z_ARRVAL_PP(hash));
	}

	return list;
}
/* }}} */

/* {{{ pip_hash_to_tuple(zval **hash)
   Convert a PHP hash to a Python tuple */
PyObject *
pip_hash_to_tuple(zval **hash)
{
	return PyList_AsTuple(pip_hash_to_list(hash));
}
/* }}} */

/* {{{ pip_hash_to_dict(zval **hash)
   Convert a PHP hash to a Python dictionary */
PyObject *
pip_hash_to_dict(zval **hash)
{
	PyObject *dict, *item, *integer;
	zval **entry;
	char *string_key;
	long num_key = 0;

	/* Make sure we were given a PHP hash */
	if (Z_TYPE_PP(hash) != IS_ARRAY) {
		return NULL;
	}

	/* Create a new empty dictionary */
	dict = PyDict_New();

	/* Let's start at the very beginning, a very good place to start. */
	zend_hash_internal_pointer_reset(Z_ARRVAL_PP(hash));

	/* Iterate over the hash's elements */
	while (zend_hash_get_current_data(Z_ARRVAL_PP(hash),
									  (void **)&entry) == SUCCESS) {

		/* Convert the PHP value to its Python equivalent (recursion) */
		item = pip_zval_to_pyobject(entry);

		/* Assign the item with the appropriate key type (string or integer) */
		switch (zend_hash_get_current_key(Z_ARRVAL_PP(hash), &string_key,
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

		/* Advance to the next entry */
		zend_hash_move_forward(Z_ARRVAL_PP(hash));
	}

	return dict;
}
/* }}} */

/* {{{ pip_zobject_to_pyobject(zval **obj)
   Convert a PHP (Zend) object to a Python object */
PyObject *
pip_zobject_to_pyobject(zval **obj)
{
	PyObject *dict, *item, *str;
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
	zend_hash_internal_pointer_reset(Z_OBJPROP_PP(obj));

	/* Iterate over the hash's elements */
	while (zend_hash_get_current_data(Z_OBJPROP_PP(obj),
									  (void **)&entry) == SUCCESS) {

		/* Convert the PHP value to its Python equivalent (recursion) */
		item = pip_zval_to_pyobject(entry);

		switch (zend_hash_get_current_key(Z_OBJPROP_PP(obj),
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
				php_error(E_ERROR, "No array key");
				break;
		}

		/* Advance to the next entry */
		zend_hash_move_forward(Z_OBJPROP_PP(obj));
	}

	return dict;
}
/* }}} */

/* {{{ pip_zval_to_pyobject(zval **val)
   Converts the given zval into an equivalent PyObject */
PyObject *
pip_zval_to_pyobject(zval **val)
{
	PyObject *ret;

	if (val == NULL) {
		return NULL;
	}

	switch (Z_TYPE_PP(val)) {
	case IS_BOOL:
		ret = Py_BuildValue("i", Z_LVAL_PP(val) ? 1 : 0);
		break;
	case IS_LONG:
		ret = Py_BuildValue("l", Z_LVAL_PP(val));
		break;
	case IS_DOUBLE:
		ret = Py_BuildValue("d", Z_DVAL_PP(val));
		break;
	case IS_STRING:
		ret = Py_BuildValue("s", Z_STRVAL_PP(val));
		break;
	case IS_ARRAY:
		ret = pip_hash_to_dict(val);
		break;
	case IS_OBJECT:
		ret = pip_zobject_to_pyobject(val);
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

/* {{{ pip_sequence_to_hash(PyObject *seq)
 * Convert a Python sequence to a PHP hash */
zval *
pip_sequence_to_hash(PyObject *seq)
{
	zval *hash, *val;
	PyObject *item;
	int i = 0;

	/* Make sure this object implements the sequence protocol */
	if (!PySequence_Check(seq)) {
		return NULL;
	}

	/* Initialize our PHP array */
	MAKE_STD_ZVAL(hash);
	if (array_init(hash) != SUCCESS) {
		return NULL;
	}

	/* Iterate over the items in the sequence */
	while (item = PySequence_GetItem(seq, i++)) {
		val = pip_pyobject_to_zval(item);

		if (zend_hash_next_index_insert(HASH_OF(hash), (void *)&val,
									   sizeof(zval *), NULL) == FAILURE) {
			php_error(E_ERROR, "Python: Array conversion error");
		}
		Py_DECREF(item);
	}

	return hash;
}
/* }}} */

/* {{{ pip_mapping_to_hash(PyObject *map)
   Convert a Python mapping to a PHP hash */
zval *
pip_mapping_to_hash(PyObject *map)
{
	zval *hash, *val;
	PyObject *keys, *key, *item;
	char *key_name;
	int i, key_len;

	if (!PyMapping_Check(map)) {
		return NULL;
	}

	/* Initialize our PHP array */
	MAKE_STD_ZVAL(hash);
	if (array_init(hash) != SUCCESS) {
		return NULL;
	}

	/* Retrieve the list of keys for this mapping */
	keys = PyMapping_Keys(map);
	if (keys == NULL) {
		return hash;
	}

	/* Iterate over the list of keys */
	for (i = 0; i < PySequence_Size(keys); i++) {
		key = PySequence_GetItem(keys, i);
		if (key) {
			/* Get the string representation of the key */
			if (pip_str(key, &key_name, &key_len) != -1) {
				/* Extract the item for this key */
				item = PyMapping_GetItemString(map, key_name);
				if (item) {
					val = pip_pyobject_to_zval(item);

					/* Add the new item to the associative array */
					if (zend_hash_update(HASH_OF(hash), key_name, key_len,
										 (void *)&val, sizeof(zval *),
										 NULL) == FAILURE) {
						php_error(E_ERROR, "Python: Array conversion error");
					}
					Py_DECREF(item);
				} else {
					php_error(E_ERROR, "Python: Could not get item for key");
				}
			} else {
				php_error(E_ERROR, "Python: Mapping key conversion error");
			}
			Py_DECREF(key);
		}
	}
	Py_DECREF(keys);

	return hash;
}
/* }}} */

/* {{{ pip_pyobject_to_zobject(PyObject *obj)
   Convert Python object to a PHP (Zend) object */
zval *
pip_pyobject_to_zobject(PyObject *obj)
{
	pval *ret;
	zval *handle;
	TSRMLS_FETCH();

	/* Create a PHP Python object */
	MAKE_STD_ZVAL(ret);
	object_init_ex(ret, &python_class_entry);
	ret->is_ref = 1;
	ret->refcount = 1;

	/* Assign the current PyObject to the new PHP Python object */
	ALLOC_ZVAL(handle);
	ZVAL_RESOURCE(handle, zend_list_insert(obj, le_pyobject));
	zval_copy_ctor(handle);
	INIT_PZVAL(handle);
	zend_hash_index_update(Z_OBJPROP_P(ret), 0, &handle, sizeof(zval *), NULL);

	return ret;
}
/* }}} */

/* {{{ pip_pyobject_to_zval(PyObject *obj)
   Converts the given PyObject into an equivalent zval */
zval *
pip_pyobject_to_zval(PyObject *obj)
{
	zval *ret;

	if (obj == NULL) {
		return NULL;
	}

	/* Initialize the return value */
	MAKE_STD_ZVAL(ret);

	if (PyInt_Check(obj)) {
		ZVAL_LONG(ret, PyInt_AsLong(obj));
	} else if (PyLong_Check(obj)) {
		ZVAL_LONG(ret, PyLong_AsLong(obj));
	} else if (PyFloat_Check(obj)) {
		ZVAL_DOUBLE(ret, PyFloat_AsDouble(obj));
	} else if (PyString_Check(obj)) {
		ZVAL_STRINGL(ret, PyString_AsString(obj), PyString_Size(obj), 1);
	} else if (obj == Py_None) {
		ZVAL_NULL(ret);
	} else if (PyTuple_Check(obj) || PyList_Check(obj)) {
		ret = pip_sequence_to_hash(obj);
	} else if (PyDict_Check(obj)) {
		ret = pip_mapping_to_hash(obj);
	} else {
		ret = pip_pyobject_to_zobject(obj);
	}

	return ret;
}
/* }}} */

/* Argument Conversions */

/* {{{ pip_args_to_tuple(int argc, int start)
   Converts PHP arguments into a Python tuple suitable for argument passing */
PyObject * pip_args_to_tuple(int argc, int start TSRMLS_DC)
{
	zval ***zargs = NULL;
	PyObject *args = NULL;

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

		/* Add each parameter to a new tuple. */
		for (i = start; i < argc; i++) {
			PyObject *arg = pip_zval_to_pyobject(zargs[i]);
			PyTuple_SetItem(args, i - start, arg);
		}
	}

	efree(zargs);

	return args;
}
/* }}} */

/* {{{ pip_args_to_tuple_ex(int ht, int argc, int start)
   Converts PHP arguments into a Python tuple suitable for argument passing */
PyObject * pip_args_to_tuple_ex(int ht, int argc, int start TSRMLS_DC)
{
	zval **zargs = NULL;
	PyObject *args = NULL;

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

		/* Add each parameter to a new tuple. */
		for (i = start; i < argc; i++) {
			PyObject *arg = pip_zval_to_pyobject(&zargs[i]);
			PyTuple_SetItem(args, i - start, arg);
		}
	}

	efree(zargs);

	return args;
}
/* }}} */

/* Object Representations */

/* {{{ pip_str(PyObject *obj, char **buffer, int *length)
   Returns the null-terminated string representation of a Python object */
int pip_str(PyObject *obj, char **buffer, int *length)
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
