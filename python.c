/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2002 The PHP Group                                |
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

/*
 * TODO:
 *	- multiple interpreters (one per request?)
 *	- use PyImport_ImportModuleEx() to set globals and locals
 *	- investigate standard common functions for all import Python objects
 *	- allow modules to be manipulated as objects from PHP
 *	- allow passing associative arrays for keyword arguments
 *	- allow setting PYTHONPATH from php.ini
 *	- preload modules from php.ini
 *	- true conversion of PHP objects
 *	- investigate ability to extend Python objects using PHP objects
 *	- safe_mode checks where applicable
 *	- display the traceback upon an exception (optionally?)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_python.h"

#include "ext/standard/php_string.h"	/* php_strtolower() */

#include "Python.h"

static int le_pyobject = 0;
static zend_class_entry python_class_entry;


/*
 * Type conversion functions
 */

/* {{{ prototypes
 */
/* Sequences */
static PyObject * convert_hash_to_list(zval **hash);
static PyObject * convert_hash_to_tuple(zval **hash);
static PyObject * convert_hash_to_dict(zval **hash);

static zval * convert_sequence_to_hash(PyObject *seq);
static zval * convert_mapping_to_hash(PyObject *map);

/* Objects */
static PyObject * convert_zobject_to_pyobject(zval **obj);
static zval * convert_pyobject_to_zobject(PyObject *obj);

/* Generic */
static PyObject * convert_zval_to_pyobject(zval **val);
static zval * convert_pyobject_to_zval(PyObject *obj);
/* }}} */

/* {{{ convert_hash_to_list(zval **hash)
   Convert a PHP hash to a Python list */
static PyObject *
convert_hash_to_list(zval **hash)
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
		item = convert_zval_to_pyobject(entry);

		/* Add this item to the list and increment the position */
		PyList_SetItem(list, pos++, item);

		/* Advance to the next entry */
		zend_hash_move_forward(Z_ARRVAL_PP(hash));
	}

	return list;
}
/* }}} */

/* {{{ convert_hash_to_tuple(zval **hash)
   Convert a PHP hash to a Python tuple */
static PyObject *
convert_hash_to_tuple(zval **hash)
{
	return PyList_AsTuple(convert_hash_to_list(hash));
}
/* }}} */

/* {{{ convert_hash_to_dict(zval **hash)
   Convert a PHP hash to a Python dictionary */
static PyObject *
convert_hash_to_dict(zval **hash)
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
		item = convert_zval_to_pyobject(entry);

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

/* {{{ convert_sequence_to_hash(PyObject *seq)
 * Convert a Python sequence to a PHP hash */
static zval *
convert_sequence_to_hash(PyObject *seq)
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
		val = convert_pyobject_to_zval(item);

		if (zend_hash_next_index_insert(HASH_OF(hash), (void *)&val,
									   sizeof(zval *), NULL) == FAILURE) {
			php_error(E_ERROR, "Python: Array conversion error");
		}
		Py_DECREF(item);
	}

	return hash;
}
/* }}} */

/* {{{ convert_mapping_to_hash(PyObject *map)
   Convert a Python mapping to a PHP hash */
static zval *
convert_mapping_to_hash(PyObject *map)
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
			if (PyString_AsStringAndSize(key, &key_name, &key_len) != -1) {
				/* Extract the item for this key */
				item = PyMapping_GetItemString(map, key_name);
				if (item) {
					val = convert_pyobject_to_zval(item);

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

/* {{{ convert_zobject_to_pyobject(zval **obj)
   Convert a PHP (Zend) object to a Python object */
static PyObject *
convert_zobject_to_pyobject(zval **obj)
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
		item = convert_zval_to_pyobject(entry);

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

/* {{{ convert_pyobject_to_zobject(PyObject *obj)
   Convert Python object to a PHP (Zend) object */
static zval *
convert_pyobject_to_zobject(PyObject *obj)
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
	Z_TYPE_P(handle) = IS_LONG;
	Z_LVAL_P(handle) = zend_list_insert(obj, le_pyobject);
	pval_copy_constructor(handle);
	INIT_PZVAL(handle);
	zend_hash_index_update(Z_OBJPROP_P(ret), 0, &handle, sizeof(pval *), NULL);

	return ret;
}
/* }}} */

/* {{{ convert_zval_to_pyobject(zval **val)
   Converts the given zval into an equivalent PyObject */
static PyObject *
convert_zval_to_pyobject(zval **val)
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
		ret = convert_hash_to_dict(val);
		break;
	case IS_OBJECT:
		ret = convert_zobject_to_pyobject(val);
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

/* {{{ convert_pyobject_to_zval(zval **val)
   Converts the given PyObject into an equivalent zval */
static zval *
convert_pyobject_to_zval(PyObject *obj)
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
		ZVAL_STRING(ret, PyString_AsString(obj), PyString_Size(obj));
	} else if (PyTuple_Check(obj) || PyList_Check(obj)) {
		ret = convert_sequence_to_hash(obj);
	} else if (PyDict_Check(obj)) {
		ret = convert_mapping_to_hash(obj);
	} else {
		ret = convert_pyobject_to_zobject(obj);
	}

	return ret;
}
/* }}} */

/*
 * PHP Module for Python
 *
 * The following functions expose various PHP functions to the Python
 * environment.  This allows the embedded Python interpreter to interact
 * with PHP at runtime.  These functions can be accessed from within Python
 * via the "php" module.
 */

/* {{{ py_php_version
 */
static PyObject *
py_php_version(PyObject *self, PyObject *args)
{
	return Py_BuildValue("s", PHP_VERSION);
}
/* }}} */

/* {{{ py_php_var
 */
static PyObject *
py_php_var(PyObject *self, PyObject *args)
{
	char *name;
	zval **data;

	TSRMLS_FETCH();

	if (!PyArg_ParseTuple(args, "s", &name)) {
		return NULL;
	}

	if (zend_hash_find(&EG(symbol_table), name, strlen(name) + 1, (void **) &data) != SUCCESS) {
		return NULL;
	}

	return convert_zval_to_pyobject(data);
}
/* }}} */

/* {{{ php_methods[]
 */
static PyMethodDef php_methods[] = {
	{"version",			py_php_version,				METH_VARARGS},
	{"var",				py_php_var,					METH_VARARGS},
	{NULL, NULL, 0, NULL}
};
/* }}} */

/*
 * Embedded Python Extension for PHP
 *
 * The following represents the PHP extension which embeds the Python
 * interpreter inside of the PHP process.  These functions allow the PHP
 * environment to interact with the Python interpreter (which in turn can
 * call PHP functions made available by the "php" module constructed above.
 */

/* Utility Functions */

/* {{{ _prepend_syspath
 */
static void
_prepend_syspath(const char *dir)
{
	if (dir) {
		PyObject *sys;
		PyObject *path;
		PyObject *dirstr;

		sys = PyImport_ImportModule("sys");
		path = PyObject_GetAttrString(sys, "path");
		dirstr = PyString_FromString(dir);

		/* Prepend dir to sys.path if it's not already there. */
		if (PySequence_Index(path, dirstr) == -1) {
			PyObject *list;
			PyErr_Clear();
			list = Py_BuildValue("[O]", dirstr);
			PyList_SetSlice(path, 0, 0, list);
			Py_DECREF(list);
		}

		Py_DECREF(dirstr);
		Py_DECREF(path);
		Py_DECREF(sys);
	}
}
/* }}} */

/* {{{ python_addpaths
 */
static void
python_addpaths(const char *paths)
{
	char *delim = ":;";
	char *path = NULL;

	path = strtok((char *)paths, delim);
	while (path != NULL) {
		_prepend_syspath(path);
		path = strtok(NULL, delim);
	}
}
/* }}} */

/* {{{ python_error
 */
static void
python_error(int error_type)
{
	PyObject *ptype, *pvalue, *ptraceback;
	PyObject *type, *value;

	/* Fetch the last error and store the type and value as strings. */
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	type = PyObject_Str(ptype);
	value = PyObject_Str(pvalue);

	Py_XDECREF(ptype);
	Py_XDECREF(pvalue);
	Py_XDECREF(ptraceback);

	/* Output the error to the user using php_error. */
	php_error(error_type, "Python: [%s] '%s'", PyString_AsString(type),
			  PyString_AsString(value));
}
/* }}} */

/* PHP Extension Stuff */

/* {{{ globals
*/
ZEND_BEGIN_MODULE_GLOBALS(python)
	PyObject *interpreters;
	char *paths;
ZEND_END_MODULE_GLOBALS(python)

ZEND_DECLARE_MODULE_GLOBALS(python)
/* }}} */

/* {{{ init_globals
 */
static void
init_globals(zend_python_globals *python_globals TSRMLS_DC)
{
	memset(python_globals, 0, sizeof(zend_python_globals));
}
/* }}} */

/* {{{ python_call_function_handler
   Function call handler */
void
python_call_function_handler(INTERNAL_FUNCTION_PARAMETERS,
							 zend_property_reference *property_reference)
{
	pval *object = property_reference->object;
	zend_overloaded_element *function_name =
		(zend_overloaded_element*)property_reference->elements_list->tail->data;

	/*
	 * If the method name is "python", this is a call to the constructor.
	 * We need to create and return a new Python object.
	 */
	if ((zend_llist_count(property_reference->elements_list) == 1) &&
		(strcmp("python", Z_STRVAL(function_name->element)) == 0)) {
		char *module_name, *object_name;
		int module_name_len, object_name_len;
		zval *arguments = NULL;
		PyObject *module;

		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|a",
								  &module_name, &module_name_len,
								  &object_name, &object_name_len,
								  &arguments) == FAILURE) {
			return;
		}

		module = PyImport_ImportModule(module_name);
		if (module != NULL) {
			PyObject *dict, *item;

			dict = PyModule_GetDict(module);
			
			item = PyDict_GetItemString(dict, object_name);
			if (item) {
				PyObject *obj, *args = NULL;
				pval *handle;

				/* Build the argument list (as a tuple) */
				if (arguments) {
					args = convert_hash_to_tuple(&arguments);
				}

				/* Create a new Python object by calling the constructor */
				obj = PyObject_CallObject(item, args);
				if (args) {
					Py_DECREF(args);
				}

				if (obj) {
					/* Wrap the Python object in a zval handle */
					ALLOC_ZVAL(handle);
					ZVAL_LONG(handle, zend_list_insert(obj, le_pyobject));
					pval_copy_constructor(handle);
					INIT_PZVAL(handle);
					zend_hash_index_update(Z_OBJPROP_P(object), 0, &handle,
										   sizeof(pval *), NULL);
				} else {
					ZVAL_NULL(object);
				}
			} else {
				python_error(E_ERROR);
			}
		} else {
			python_error(E_ERROR);
		}
	} else {
		PyObject *obj, *dir;
		pval **handle;
		int type;

		/* Fetch and reinstate the Python object from the hash */
		zend_hash_index_find(Z_OBJPROP_P(object), 0, (void **) &handle);
		obj = (PyObject *) zend_list_find(Z_LVAL_PP(handle), &type);

		/* Make sure the object exists and is callable */
		if (obj && (dir = PyObject_Dir(obj))) {
			PyObject *map, *item, *method = NULL;
			int i, key_len;
			char *key;

			/*
			 * Because PHP returns the function name in its lowercased form,
			 * and Python is a case-sensitive language, we need to build a
			 * map of the available Python methods keyed by their lowercased
			 * rendering.
			 *
			 * To do so, we use a Python dictionary.  We could also use a Zend
			 * HashTable, but Python dictionaries are easier to implement.
			 *
			 * Once the ability to enable case-sensitivity in PHP exists,
			 * this kludge can be removed (or at least only enabled
			 * conditionally).
			 */
			map = PyDict_New();
			for (i = 0; i < PyList_Size(dir); i++) {
				item = PyList_GetItem(dir, i);

				/*
				 * PyString_AsString returns a pointer to its internal string
				 * buffer, so the string can't be modified.  php_strtolower
				 * modifies the string, so we must make a copy.
				 */
				key = estrdup(PyString_AsString(item));
				key_len = PyString_Size(item);
				
				PyDict_SetItemString(map, php_strtolower(key, key_len), item);
				efree(key);
			}
			Py_DECREF(dir);

			/* Attempt to pull the requested method name out of the map */
			item = PyDict_GetItemString(map, Z_STRVAL(function_name->element));
			Py_DECREF(map);

			/* Get a pointer to the method object if the name was found */
			if (item) {
				method = PyObject_GetAttr(obj, item);
			}

			/* If the method exists and is callable ... */
			if (method && PyCallable_Check(method)) {
				PyObject *args, *result = NULL;
				int argc = ZEND_NUM_ARGS();
				pval **argv;

				/* Build the argument list */
				argv = (pval **) emalloc(sizeof(pval *) * argc);
				zend_get_parameters_array(ht, argc, argv);

				args = PyTuple_New(argc);
				for (i = 0; i < argc; i++) {
					PyTuple_SetItem(args,i,convert_zval_to_pyobject(&argv[i]));
				}
				efree(argv);

				/* Invoke the requested method on the object */
				result = PyObject_CallObject(method, args);
				Py_DECREF(method);
				Py_DECREF(args);

				if (result != NULL) {
					/* Convert the Python result to its PHP equivalent */
					*return_value = *convert_pyobject_to_zval(result);
					zval_copy_ctor(return_value);
					Py_DECREF(result);
				} else {
					python_error(E_ERROR);
				}
			} else {
				python_error(E_ERROR);
			}
		} else {
			python_error(E_ERROR);
		}
	}

	pval_destructor(&function_name->element);
}
/* }}} */

/* {{{ python_attribute_handler
 */
static pval
python_attribute_handler(zend_property_reference *property_reference,
						 PyObject *value TSRMLS_DC)
{
	zend_llist_element *element;
	zend_overloaded_element *property;
	pval **handle, result;
	PyObject *obj;
	int type;

	/* Get the property name */
	element = property_reference->elements_list->head;
	property = (zend_overloaded_element *) element->data;

	/* Default to a NULL result */
	Z_TYPE(result) = IS_NULL;

	/* Fetch and reinstate the Python object from the hash */
	zend_hash_index_find(Z_OBJPROP_P(property_reference->object), 0,
						 (void **) &handle);
	obj = zend_list_find(Z_LVAL_PP(handle), &type);

	/* Make sure we have a valid Python object */
	if (type == le_pyobject) {
		char *prop = Z_STRVAL(property->element);

		/*
		 * If a value was provided, perform a "set" operation.
		 * Otherwise, perform a "get" operation.
		 */
		if (value) {
			if (PyObject_SetAttrString(obj, prop, value) != -1) {
				ZVAL_TRUE(&result);
			}
		} else {
			PyObject *attr = NULL;

			if (attr = PyObject_GetAttrString(obj, prop)) {
				result = *(convert_pyobject_to_zval(attr));
				Py_DECREF(attr);
			}
		}
	} else {
		php_error(E_ERROR, "Not a Python object");
	}

	pval_destructor(&property->element);

	return result;
}
/* }}} */

/* {{{ python_get_property_handler
   Get property handler */
pval
python_get_property_handler(zend_property_reference *property_reference)
{
	TSRMLS_FETCH();

	return python_attribute_handler(property_reference, NULL TSRMLS_CC);
}
/* }}} */

/* {{{ python_set_property_handler
   Set property handler */
int
python_set_property_handler(zend_property_reference *property_reference,
							pval *value)
{
	PyObject *val = NULL;
	pval result;
	TSRMLS_FETCH();

	/* Convert the PHP value to its Python equivalent */
	val = convert_zval_to_pyobject(&value);

	result = python_attribute_handler(property_reference, val TSRMLS_CC);
	if (Z_TYPE(result) == IS_NULL) {
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ python_functions[]
 */
function_entry python_functions[] = {
	PHP_FE(py_eval,			NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ python_module_entry
 */
zend_module_entry python_module_entry = {
	STANDARD_MODULE_HEADER,
	"python",
	python_functions,
	PHP_MINIT(python),
	PHP_MSHUTDOWN(python),
	NULL,
	NULL,
	PHP_MINFO(python),
	NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_W32API
ZEND_GET_MODULE(python)
#endif
/* }}} */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("python.paths",		".",		PHP_INI_ALL,	OnUpdateString,		paths,          zend_python_globals,   python_globals)
PHP_INI_END()
/* }}} */

/* {{{ python_destructor
 */
static void python_destructor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	PyObject *obj = (PyObject *)rsrc->ptr;

	Py_DECREF(obj);
}
/* }}} */
	
/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(python)
{
	/* Initialize and register the overloaded "python" object class type */
	INIT_OVERLOADED_CLASS_ENTRY(
		python_class_entry,					/* Class container */
		"python",							/* Class name */
		NULL,								/* Functions */
		python_call_function_handler,		/* Function call handler */
		python_get_property_handler,		/* Get property handler */
		python_set_property_handler);		/* Set property handler */

	zend_register_internal_class(&python_class_entry TSRMLS_CC);

	/* Register the Python object destructor */
	le_pyobject = zend_register_list_destructors_ex(python_destructor, NULL,
													"python", module_number);

	/* Initialize global variables and INI file entries */
	ZEND_INIT_MODULE_GLOBALS(python, init_globals, NULL);
	REGISTER_INI_ENTRIES();

	/*
	 * XXX What kind of performance hit are we taking here?  Maybe this
	 * should only be done upon the first Python-related request.
	 */

	/* Initialize the Python interpreter */
	Py_Initialize();

	/* Initialize the PHP module for Python (see above) */
	Py_InitModule3("php", php_methods, "PHP Module");

	/* Add the current directory to Python's sys.path list */
	python_addpaths(PYG(paths));

	return Py_IsInitialized() ? SUCCESS : FAILURE;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(python)
{
	/* Unregister INI file entries */
	UNREGISTER_INI_ENTRIES();

	/* Shut down the Python interface */
	Py_Finalize();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(python)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Python Support", "enabled");
	php_info_print_table_row(2, "Python Version", PY_VERSION);
	php_info_print_table_end();
}
/* }}} */

/* PHP Python Functions */

/* {{{ proto void py_eval(string code)
   Evaluate the given string of code by passing it to the interpreter */
PHP_FUNCTION(py_eval)
{
	char *code;
	int code_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
							  &code, &code_len) == FAILURE) {
		return;
	}

	PyRun_SimpleString(code); 
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
