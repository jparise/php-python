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
 *	- preload modules from php.ini
 *	- true conversion of PHP objects
 *	- investigate ability to extend Python objects using PHP objects
 *	- safe_mode checks where applicable
 *	- display the traceback upon an exception (optionally?)
 *	- redirect Python stderr back to PHP
 *	- check for attribute existence in python_attribute_handler()
 *	- var_dump($python_object) should list attributes (and methods)
 *	- py_import() should accept arrays and support 'from foo import bar'
 *	- Implement: Py_SetProgramName() and PySys_SetArgv().
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_python.h"
#include "pip_convert.h"

#include "ext/standard/php_string.h"	/* php_strtolower() */

#include "Python.h"

int le_pyobject = 0;
zend_class_entry python_class_entry;

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

	return pip_zval_to_pyobject(data);
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

/* {{{ _syspath_prepend
 */
static int
_syspath_prepend(const char *dir)
{
	PyObject *sys, *path, *dirstr;

	if (dir == NULL) {
		return -1;
	}

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

	return 0;
}
/* }}} */

/* {{{ _syspath_prepend_list
 */
static void
_syspath_prepend_list(const char *pathlist)
{
	char *delim = ":;";
	char *path = NULL;

	path = strtok((char *)pathlist, delim);
	while (path != NULL) {
		_syspath_prepend(path);
		path = strtok(NULL, delim);
	}
}
/* }}} */

/* {{{ _syspath_append
 */
static int
_syspath_append(const char *dir)
{
	PyObject *sys, *path, *dirstr;

	if (dir == NULL) {
		return -1;
	}

	sys = PyImport_ImportModule("sys");
	path = PyObject_GetAttrString(sys, "path");
	dirstr = PyString_FromString(dir);

	/* Append dir to sys.path if it's not already there. */
	if (PySequence_Index(path, dirstr) == -1) {
		PyErr_Clear();
		PyList_Append(path, dirstr);
	}

	Py_DECREF(dirstr);
	Py_DECREF(path);
	Py_DECREF(sys);

	return 0;
}
/* }}} */

/* {{{ _syspath_append_list
 */
static void
_syspath_append_list(const char *pathlist)
{
	char *delim = ":;";
	char *path = NULL;

	path = strtok((char *)pathlist, delim);
	while (path != NULL) {
		_syspath_append(path);
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
	char *prepend_path;
	char *append_path;
ZEND_END_MODULE_GLOBALS(python)

ZEND_DECLARE_MODULE_GLOBALS(python)
/* }}} */

/* {{{ init_globals
 */
static void
init_globals(zend_python_globals *python_globals TSRMLS_DC)
{
	/* Zero out the entire python_globals structure. */
	memset(python_globals, 0, sizeof(zend_python_globals));
}
/* }}} */

/* {{{ python_call_function_handler
   Function call handler */
void
python_call_function_handler(INTERNAL_FUNCTION_PARAMETERS,
							 zend_property_reference *property_reference)
{
	zval *object = property_reference->object;
	zend_overloaded_element *function_name =
		(zend_overloaded_element*)property_reference->elements_list->tail->data;

	/*
	 * If the method name is "python", this is a call to the constructor.
	 * We need to create and return a new Python object.
	 */
	if ((zend_llist_count(property_reference->elements_list) == 1) &&
		(strcmp("python", Z_STRVAL(function_name->element)) == 0)) {
		char *module_name, *class_name;
		int module_name_len, class_name_len;
		PyObject *module;
		int argc = ZEND_NUM_ARGS();

		/* Parse only the first two arguments (module name and class name). */
		if (zend_parse_parameters(2 TSRMLS_CC, "ss", &module_name, &module_name_len,
								  &class_name, &class_name_len) == FAILURE) {
			return;
		}

		module = PyImport_ImportModule(module_name);
		if (module != NULL) {
			PyObject *dict, *item;

			dict = PyModule_GetDict(module);
			
			item = PyDict_GetItemString(dict, class_name);
			if (item) {
				PyObject *obj, *args = NULL;
				zval *handle;

				/* Create a new Python object by calling the constructor */
				args = pip_args_to_tuple(argc, 2 TSRMLS_CC);
				obj = PyObject_CallObject(item, args);
				if (args) {
					Py_DECREF(args);
				}

				if (obj) {
					/* Store the Python object handle as a Zend resource */
					ALLOC_ZVAL(handle);
					ZVAL_RESOURCE(handle, zend_list_insert(obj, le_pyobject));
					zval_copy_ctor(handle);
					INIT_PZVAL(handle);
					zend_hash_index_update(Z_OBJPROP_P(object), 0, &handle,
										   sizeof(zval *), NULL);
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
		zval **handle;

		/* Fetch and reinstate the Python object from the hash */
		zend_hash_index_find(Z_OBJPROP_P(object), 0, (void **) &handle);
		ZEND_FETCH_RESOURCE(obj, PyObject *, handle, -1, "PyObject",
							le_pyobject);

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

				/* Invoke the requested method on the object */
				args = pip_args_to_tuple_ex(ht, ZEND_NUM_ARGS(), 0 TSRMLS_CC);
				result = PyObject_CallObject(method, args);
				Py_DECREF(method);
				if (args) {
					Py_DECREF(args);
				}

				if (result != NULL) {
					/* Convert the Python result to its PHP equivalent */
					*return_value = *pip_pyobject_to_zval(result);
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
	zval **handle, *object, return_value;
	PyObject *obj;
	char *propname;

	/* Get the attribute values */
	object = property_reference->object;
	element = property_reference->elements_list->head;
	property = (zend_overloaded_element *) element->data;
	propname = Z_STRVAL(property->element);

	/* Default to a NULL result */
	INIT_ZVAL(return_value);
	ZVAL_NULL(&return_value);

	/* Fetch and reinstate the Python object from the hash */
	zend_hash_index_find(Z_OBJPROP_P(object), 0, (void **) &handle);
	obj = (PyObject *) zend_fetch_resource(handle TSRMLS_CC, -1, "PyObject",
										   NULL, 1, le_pyobject);

	/*
	 * If a value was provided, perform a "set" operation.
	 * Otherwise, perform a "get" operation.
	 */
	if (value) {
		if (PyObject_SetAttrString(obj, propname, value) != -1) {
			ZVAL_TRUE(&return_value);
		}
	} else {
		PyObject *attr = NULL;

		if (attr = PyObject_GetAttrString(obj, propname)) {
			return_value = *pip_pyobject_to_zval(attr);
			Py_DECREF(attr);
		}
	}

	pval_destructor(&property->element);

	return return_value;
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
	val = pip_zval_to_pyobject(&value);

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
	PHP_FE(py_path,			NULL)
	PHP_FE(py_path_prepend,	NULL)
	PHP_FE(py_path_append,	NULL)
	PHP_FE(py_import,		NULL)
	PHP_FE(py_eval,			NULL)
	PHP_FE(py_call,			NULL)
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

#ifdef COMPILE_DL_PYTHON
ZEND_GET_MODULE(python)
#endif
/* }}} */

/* {{{ OnUpdatePrependPath
 */
static PHP_INI_MH(OnUpdatePrependPath)
{
	/* Always set the global variable. */
	PYG(prepend_path) = new_value;

	/* sys.path can only be set after the interpreter has been initialized. */
	if (Py_IsInitialized()) {
		_syspath_prepend_list(new_value);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ OnUpdateAppendPath
 */
static PHP_INI_MH(OnUpdateAppendPath)
{
	/* Always set the global variable. */
	PYG(append_path) = new_value;

	/* sys.path can only be set after the interpreter has been initialized. */
	if (Py_IsInitialized()) {
		_syspath_append_list(new_value);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	PHP_INI_ENTRY("python.prepend_path",	".",	PHP_INI_ALL,	OnUpdatePrependPath)
	PHP_INI_ENTRY("python.append_path",		"",		PHP_INI_ALL,	OnUpdateAppendPath)
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
	/* Initialize and register the overloaded "Python" object class type */
	INIT_OVERLOADED_CLASS_ENTRY(
		python_class_entry,					/* Class container */
		"Python",							/* Class name */
		NULL,								/* Functions */
		python_call_function_handler,		/* Function call handler */
		python_get_property_handler,		/* Get property handler */
		python_set_property_handler);		/* Set property handler */

	zend_register_internal_class(&python_class_entry TSRMLS_CC);

	/* Register the Python object destructor */
	le_pyobject = zend_register_list_destructors_ex(python_destructor, NULL,
													"Python", module_number);

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

	/* Explicitly add the python.prepend_path and python.append_path values. */
	_syspath_prepend_list(PYG(prepend_path));
	_syspath_append_list(PYG(append_path));

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
	php_info_print_table_row(2, "Python Version", Py_GetVersion());
	php_info_print_table_row(2, "Extension Version", "$Revision$");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();

	php_info_print_table_start();
	php_info_print_table_header(1, "Python Copyright");
	php_info_print_box_start(0);
	php_printf("%s", Py_GetCopyright());
	php_info_print_box_end();
	php_info_print_table_end();
}
/* }}} */

/* PHP Python Functions */

/* {{{ proto void py_path()
   Return the value of sys.path as an array. */
PHP_FUNCTION(py_path)
{
	PyObject *sys = NULL, *path = NULL;

	/* Attempt to import the "sys" module. */
	sys = PyImport_ImportModule("sys");
	if (sys == NULL) {
		RETURN_FALSE;
	}

	/* Acquire a reference to the "path" list of the "sys" module. */
	path = PyObject_GetAttrString(sys, "path");
	Py_DECREF(sys);
	if (path == NULL) {
		RETURN_FALSE;
	}

	/* Convert the Python list to a PHP array */
	*return_value = *pip_sequence_to_hash(path);
	zval_copy_ctor(return_value);

	Py_DECREF(path);
}
/* }}} */

/* {{{ proto void py_path_prepend(string dir)
   Prepend the given directory to the sys.path list. */ 
PHP_FUNCTION(py_path_prepend)
{
	char *dir;
	int dir_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
							  &dir, &dir_len) == FAILURE) {
		return;
	}

	if (_syspath_prepend(dir) == -1) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void py_path_append(string dir)
   Append the given directory to the sys.path list. */
PHP_FUNCTION(py_path_append)
{
	char *dir;
	int dir_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
							  &dir, &dir_len) == FAILURE) {
		return;
	}

	if (_syspath_append(dir) == -1) {
		RETURN_FALSE;
	}


	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void py_import(string module)
   Import the requested module. */
PHP_FUNCTION(py_import)
{
	char *module_name, *command = NULL;
	int module_name_len, command_len, result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
							  &module_name, &module_name_len) == FAILURE) {
		return;
	}

	command_len = sizeof("import ") + module_name_len + 1;

	command = (char *) emalloc(sizeof(char) * command_len);
	if (command == NULL) {
		php_error(E_ERROR, "Memory allocation failure");
		RETURN_FALSE;
	}

	snprintf(command, command_len, "import %s", module_name);

	result = PyRun_SimpleString(command);
	efree(command);

	if (result == -1) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void py_eval(string code)
   Evaluate the given string of code by passing it to the Python interpreter */
PHP_FUNCTION(py_eval)
{
	char *code;
	int code_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
							  &code, &code_len) == FAILURE) {
		return;
	}

	if (PyRun_SimpleString(code) == -1) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void py_call(string module, string function[, array arguments])
   Call the requested function in the requested module. */
PHP_FUNCTION(py_call)
{
	char *module_name, *function_name;
	int module_name_len, function_name_len;
	PyObject *module = NULL;
	int argc = ZEND_NUM_ARGS();

	/* Parse only the first two parameters (module name and function name). */
	if (zend_parse_parameters(2 TSRMLS_CC, "ss", &module_name, &module_name_len,
							  &function_name, &function_name_len) == FAILURE) {
		return;
	}

	module = PyImport_ImportModule(module_name);
	if (module) {
		PyObject *dict, *function;

		dict = PyModule_GetDict(module);
		
		function = PyDict_GetItemString(dict, function_name);
		if (function) {
			PyObject *args = NULL, *result = NULL;

			/* Call the function with a tuple of arguments */
			args = pip_args_to_tuple(argc, 2 TSRMLS_CC);
			result = PyObject_CallObject(function, args);

			Py_DECREF(function);
			if (args) {
				Py_DECREF(args);
			}

			if (result) {
				/* Convert the Python result to its PHP equivalent */
				*return_value = *pip_pyobject_to_zval(result);
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
/* }}} */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
