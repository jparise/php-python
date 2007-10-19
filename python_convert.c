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

#include "php.h"
#include "php_python_internal.h"

extern zend_class_entry *python_class_entry;

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

/* {{{ pip_sequence_to_hash(PyObject *o, HashTable *ht TSRMLS_DC)
   Convert a Python sequence to a PHP hash. */
int
pip_sequence_to_hash(PyObject *o, HashTable *ht TSRMLS_DC)
{
	PyObject *item;
	zval *v;
	int i, size;

	/* Make sure this object implements the sequence protocol. */
	if (!PySequence_Check(o))
		return FAILURE;

	size = PySequence_Size(o);
	for (i = 0; i < size; ++i) {
		/*
		 * Acquire a reference to the current item in the sequence.  If we
		 * fail to get the object, we return a hard failure.
		 */
		item = PySequence_ITEM(o, i);
		if (item == NULL)
			return FAILURE;

		/*
		 * Attempt to convert the item from a Python object to a PHP value.
		 * This involves allocating a new zval, testing the success of the
		 * conversion, and potentially cleaning up the zval upon failure.
		 */
		MAKE_STD_ZVAL(v);
		if (pip_pyobject_to_zval(item, v TSRMLS_CC) == FAILURE) {
			zval_dtor(v);
			Py_DECREF(item);
			return FAILURE;
		}
		Py_DECREF(item);

		/*
		 * Append (i.e., insert into the next slot) the PHP value into our
		 * hashtable.  Failing to insert the value results in a hard
		 * failure.
		 */
		if (zend_hash_next_index_insert(ht, (void *)&v, sizeof(zval *),
										NULL) == FAILURE)
			return FAILURE;
	}

	return SUCCESS;
}
/* }}} */
/* {{{ pip_sequence_to_array(PyObject *o, zval *zv TSRMLS_DC)
   Convert a Python sequence to a PHP array. */
int
pip_sequence_to_array(PyObject *o, zval *zv TSRMLS_DC)
{
	/*
	 * Initialize our zval as an array.  The converted sequence will be
	 * stored in the array's hashtable.
	 */
	if (array_init(zv) == SUCCESS)
		return pip_sequence_to_hash(o, Z_ARRVAL_P(zv) TSRMLS_CC);

	return FAILURE;
}
/* }}} */
/* {{{ pip_mapping_to_hash(PyObject *o, HashTable *ht TSRMLS_DC)
   Convert a Python mapping to a PHP hash. */
int
pip_mapping_to_hash(PyObject *o, HashTable *ht TSRMLS_DC)
{
	PyObject *keys, *key, *str, *item;
	zval *v;
	char *name;
	int i, size, name_len;
	int status = FAILURE;

	/*
	 * We start by retrieving the list of keys for this mapping.  We will
	 * use this list below to address each item in the mapping.
	 */
	keys = PyMapping_Keys(o);
	if (keys) {
		/*
		 * Iterate over all of the mapping's keys.
		 */
		size = PySequence_Size(keys);
		for (i = 0; i < size; ++i) {
			/*
			 * Acquire a reference to the current item in the mapping.  If
			 * we fail to get the object, we return a hard failure.
			 */
			key = PySequence_ITEM(keys, i);
			if (key == NULL) {
				status = FAILURE;
				break;
			}

			/*
			 * We request the string representation of this key.  PHP
			 * hashtables only support string-based keys.
			 */
			str = PyObject_Str(key);
			if (!str || PyString_AsStringAndSize(str, &name, &name_len) == -1) {
				Py_DECREF(key);
				status = FAILURE;
				break;
			}

			/*
			 * Extract the item associated with the named key.
			 */
			item = PyMapping_GetItemString(o, name);
			if (item == NULL) {
				Py_DECREF(str);
				Py_DECREF(key);
				status = FAILURE;
				break;
			}

			/*
			 * Attempt to convert the item from a Python object to a PHP
			 * value.  This involves allocating a new zval.  If the
			 * conversion fails, we must remember to free the allocated zval
			 * below.
			 */
			MAKE_STD_ZVAL(v);
			status = pip_pyobject_to_zval(item, v TSRMLS_CC);

			/*
			 * If we've been successful up to this point, attempt to add the
			 * new item to our hastable.
			 */
			if (status == SUCCESS)
				status = zend_hash_update(ht, name, name_len + 1,
										  (void *)&v, sizeof(zval *), NULL);

			/*
			 * Release our reference to the Python objects that are still
			 * active in this scope.
			 */
			Py_DECREF(item);
			Py_DECREF(str);
			Py_DECREF(key);

			/*
			 * If we've failed to convert and insert this item, free our
			 * allocated zval and break out of our loop with a failure
			 * status.
			 */
			if (status == FAILURE) {
				zval_dtor(v);
				break;
			}
		}

		Py_DECREF(keys);
	}

	return status;
}
/* }}} */
/* {{{ pip_mapping_to_array(PyObject *o, zval *zv TSRMLS_DC)
   Convert a Python mapping to a PHP array. */
int
pip_mapping_to_array(PyObject *o, zval *zv TSRMLS_DC)
{
	if (array_init(zv) == SUCCESS)
		return pip_mapping_to_hash(o, Z_ARRVAL_P(zv) TSRMLS_CC);

	return FAILURE;
}
/* }}} */
/* {{{ pip_pyobject_to_zobject(PyObject *o, zval *zv TSRMLS_DC)
   Convert Python object to a PHP (Zend) object */
int
pip_pyobject_to_zobject(PyObject *o, zval *zv TSRMLS_DC)
{
	php_python_object *pip;

	/* Create a new instance of a PHP Python object. */
	if (object_init_ex(zv, python_class_entry) != SUCCESS)
		return FAILURE;

	/*
	 * Fetch the php_python_object data for this object instance. Bump the
	 * reference count of our Python object and associate it with our PHP
	 * Python object instance.
	 */
	pip = (php_python_object *)zend_object_store_get_object(zv TSRMLS_CC);
	Py_INCREF(o);
	pip->object = o;

	return SUCCESS;
}
/* }}} */
/* {{{ pip_pyobject_to_zval(PyObject *o, zval *zv TSRMLS_DC)
   Converts the given PyObject into an equivalent zval. */
int
pip_pyobject_to_zval(PyObject *o, zval *zv TSRMLS_DC)
{
	/*
	 * The general approach taken below is to infer the Python object's type
	 * using a series of tests based on Python's type-specific _Check()
	 * functions.  If the test passes, then we proceed immediately with the
	 * fastest possible conversion (which implies using the versions of the
	 * conversion functions that don't perform any additional error
	 * checking).
	 *
	 * Complex conversions are farmed out to reusable helper functions.
	 *
	 * The order of the tests below is largel insignificant, aside from the
	 * initial test of (o == NULL).  They could be reordered for performance
	 * purposes in the future once we have a sense of the most commonly
	 * converted types.
	 */

	/*
	 * If our object is invalid or None, treat it like PHP's NULL value.
	 */
	if (o == NULL || o == Py_None) {
		ZVAL_NULL(zv);
		return SUCCESS;
	}

	/*
	 * Python integers and longs (and their subclasses) are treated as PHP
	 * longs.  We don't perform any kind of fancy type casting here; if the
	 * original object isn't already an integer type, we don't attempt to
	 * treat it like one.
	 */
	if (PyInt_Check(o)) {
		ZVAL_LONG(zv, PyInt_AS_LONG(o));
		return SUCCESS;
	}
	if (PyLong_Check(o)) {
		ZVAL_LONG(zv, PyLong_AsLong(o));
		return SUCCESS;
	}

	/*
	 * Python floating point objects are treated as PHP doubles.
	 */
	if (PyFloat_Check(o)) {
		ZVAL_DOUBLE(zv, PyFloat_AS_DOUBLE(o));
		return SUCCESS;
	}

	/*
	 * Python strings are converted directly to PHP strings.  The contents
	 * of the string is copied (i.e., duplicated) into the zval.
	 */
	if (PyString_Check(o)) {
		ZVAL_STRINGL(zv, PyString_AS_STRING(o), PyString_GET_SIZE(o), 1);
		return SUCCESS;
	}

	/*
	 * Python Unicode strings are converted to ASCII and stored as PHP
	 * strings.  It is possible for this encoding-based conversion to fail.
	 * The contents of the ASCII-encoded string as copied (i.e., duplicated)
	 * into the zval.
	 *
	 * TODO:
	 * - Support richer conversions if PHP's Unicode support is available.
	 * - Potentially break this conversion out into its own routine.
	 */
	if (PyUnicode_Check(o)) {
		PyObject *s = PyUnicode_AsASCIIString(o);
		if (s) {
			ZVAL_STRINGL(zv, PyString_AS_STRING(s), PyString_GET_SIZE(s), 1);
			Py_DECREF(s);
			return SUCCESS;
		}

		return FAILURE;
	}

	/*
	 * Python objects that follow the sequence or mapping protocols are
	 * converted to PHP arrays.  Remember that PHP arrays are essentially
	 * hashtables that can be treated as simple array-like containers.
	 */
	if (PySequence_Check(o))
		return pip_sequence_to_array(o, zv TSRMLS_CC);

	if (PyMapping_Check(o))
		return pip_mapping_to_array(o, zv TSRMLS_CC);

	/*
	 * If all of the other conversions failed, we attempt to convert the
	 * Python object to a PHP object.
	 */
	return pip_pyobject_to_zobject(o, zv TSRMLS_CC);
}
/* }}} */

/* Argument Conversions */

/* {{{ pip_args_to_tuple(int argc, int start TSRMLS_DC)
   Converts PHP arguments into a Python tuple suitable for argument passing. */
PyObject *
pip_args_to_tuple(int argc, int start TSRMLS_DC)
{
	PyObject *arg, *args = NULL;
	zval ***zargs;
	int i;

	if (argc < start)
		return NULL;

	/*
	 * In order to convert our current arguments to a Python tuple object,
	 * we need to make a temporary copy of the argument array.  We use this
	 * array copy to create and populate a new tuple object containing the
	 * individual arguments.
	 *
	 * An alternative to the code below is zend_copy_parameters_array().
	 * That function populates a PHP array instance with the parameters.  It
	 * seems like the code below is probably going to be more efficient
	 * because it pre-allocates all of the memory for the array up front,
	 * but it would be worth testing the two approaches to know for sure.
	 */
	zargs = (zval ***) emalloc(sizeof(zval **) * argc);
	if (zargs) {
		if (zend_get_parameters_array_ex(argc, zargs) == SUCCESS) {
			args = PyTuple_New(argc - start);
			if (args) {
				for (i = start; i < argc; ++i) {
					arg = pip_zval_to_pyobject(*zargs[i] TSRMLS_CC);
					PyTuple_SetItem(args, i - start, arg);
				}
			}
		}

		efree(zargs);
	}

	return args;
}
/* }}} */

/* Object Representations */

/* {{{ python_str(PyObject *o, char **buffer, int *length)
   Returns the NUL-terminated string representation of a Python object. */
int
python_str(PyObject *o, char **buffer, int *length)
{
	PyObject *str;
	int ret = -1;

	/*
	 * Compute the string representation of the given object.  This is the
	 * equivalent of 'str(obj)' or passing the object to 'print'.
	 */
 	str = PyObject_Str(o);

	if (str) {
		/* XXX: length is a Py_ssize_t and could overflow an int. */
		ret = PyString_AsStringAndSize(str, buffer, length);
		Py_DECREF(str);

		/*
		 * If the conversion raised a TypeError, clear it and just return
		 * our simple failure code to the caller.
		 */
		if (ret == -1 && PyErr_ExceptionMatches(PyExc_TypeError))
			PyErr_Clear();
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
