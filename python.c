/*
 * Python in PHP - Embedded Python Extension
 *
 * Copyright (c) 2003,2004,2005,2006,2007,2008 Jon Parise <jon@php.net>
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
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_python.h"
#include "php_python_internal.h"

ZEND_DECLARE_MODULE_GLOBALS(python)

zend_class_entry *python_class_entry;

/* {{{ python_functions[]
 */
function_entry python_functions[] = {
	PHP_FE(python_version,		NULL)
	PHP_FE(python_eval,			NULL)
	PHP_FE(python_exec,			NULL)
	PHP_FE(python_call,			NULL)
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
	PHP_RINIT(python),
	PHP_RSHUTDOWN(python),
	PHP_MINFO(python),
	PHP_PYTHON_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PYTHON
ZEND_GET_MODULE(python)
#endif
/* }}} */
/* {{{ php_python_constructor_function
 */
zend_internal_function php_python_constructor = {
	ZEND_INTERNAL_FUNCTION,		/* type */
	"__construct",				/* function_name */
	NULL,						/* scope */
	0,							/* fn_flags */
	NULL,						/* prototype */
	0,							/* num_args */
	2,							/* required_num_args */
	NULL,						/* arg_info */
	0,							/* pass_rest_by_reference */
	0,							/* return_reference */
	ZEND_FN(python_construct),	/* handler */
	NULL						/* module */
};
/* }}} */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
PHP_INI_ENTRY("python.optimize", "0", PHP_INI_SYSTEM, NULL)
PHP_INI_END()
/* }}} */

/* {{{ python_init_globals(zend_python_globals *globals)
 */
static void
python_init_globals(zend_python_globals *globals)
{
	memset(globals, 0, sizeof(zend_python_globals));
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(python)
{
	zend_class_entry ce;

	ZEND_INIT_MODULE_GLOBALS(python, python_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	REGISTER_STRING_CONSTANT("PHP_PYTHON_VERSION", PHP_PYTHON_VERSION,
						     CONST_CS | CONST_PERSISTENT);

	INIT_CLASS_ENTRY(ce, "Python", NULL);
	python_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	python_class_entry->create_object = python_object_create;
	python_class_entry->constructor = (zend_function *)&php_python_constructor;

	/*
	 * We need to set up any flags before we initialize Python.  Note that we
	 * always skip signal handler registration to avoid interfering with PHP's
	 * handlers.
	 */
	Py_OptimizeFlag = INI_INT("python.optimize");
	Py_IgnoreEnvironmentFlag = 0;

	/*
	 * Initialize the embedded Python interpreter and its threading subsystem.
	 */
	Py_InitializeEx(0);
	PyEval_InitThreads();

	PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();

	/*
	 * At this point, the embedded Python interpreter should be initialized
	 * and ready to go.  If it isn't, we're in bad shape and return failure.
	 */
	return Py_IsInitialized() ? SUCCESS : FAILURE;
}
/* }}} */
/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(python)
{
	PyThreadState *tstate;

	UNREGISTER_INI_ENTRIES();

	/*
	 * Create a new thread state so we can run Py_Finalize().  This gives us a
	 * context in which to run the exit functions.
	 */
	tstate = Py_NewInterpreter();

	/*
	 * Shut down the embedded Python interpreter.  This will destroy all of
	 * the sub-interpreters and (ideally) free all of the memory allocated
	 * by Python.
	 */
	Py_Finalize();

	/*
	 * Swap out our temporary state and release our lock.  We're now totally
	 * done with the Python system.
	 */
	PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();

	return SUCCESS;
}
/* }}} */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(python)
{
	PyThreadState *tstate = NULL;

	PyEval_AcquireLock();

	/*
	 * Create a new interpreter for this request.  If we fail to do so, we
	 * can't really proceed, so we throw a fatal error.
	 */
	tstate = Py_NewInterpreter();
	if (!tstate)
	{
		php_error(E_ERROR, "Python: Failed to create new interpreter");
		return FAILURE;
	}

	/*
	 * Register all of our Python modules in this interpreter's environment.
	 */
	python_php_init();

	PYG(tstate) = tstate;

	PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();

	return SUCCESS;
}
/* }}} */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(python)
{
	PyThreadState *tstate;

	tstate = PYG(tstate);
	PyEval_AcquireThread(tstate);
	Py_EndInterpreter(tstate);

	return SUCCESS;
}
/* }}} */
/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(python)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Python Support", "enabled");
	php_info_print_table_row(2, "Python Version", Py_GetVersion());
	php_info_print_table_row(2, "Extension Version", PHP_PYTHON_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();

	php_info_print_table_start();
	php_info_print_table_header(1, "Python Environment");
	php_info_print_table_row(2, "Module Search Path", Py_GetPath());
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(1, "Python Copyright");
	php_info_print_box_start(0);
	php_printf("%s\n", Py_GetCopyright());
	php_info_print_box_end();
	php_info_print_table_end();
}
/* }}} */

/* {{{ python_error(int error_type TSRMLS_DC)
 */
static void
python_error(int error_type TSRMLS_DC)
{
	PyObject *ptype, *pvalue, *ptraceback;
	PyObject *type, *value;

	PHP_PYTHON_THREAD_ASSERT();

	/* Fetch the last error and store the type and value as strings. */
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	type = PyObject_Str(ptype);
	value = PyObject_Str(pvalue);

	Py_XDECREF(ptype);
	Py_XDECREF(pvalue);
	Py_XDECREF(ptraceback);

	if (type && value) {
		php_error(error_type, "Python: [%s] '%s'", PyString_AsString(type),
				  PyString_AsString(value));
		Py_DECREF(type);
		Py_DECREF(value);
	}
}
/* }}} */

/* {{{ proto string python_version()
   Returns the Python interpreter's version as a string. */
PHP_FUNCTION(python_version)
{
	const char *version;

	PHP_PYTHON_THREAD_ACQUIRE();
	version = Py_GetVersion();
	PHP_PYTHON_THREAD_RELEASE();

	RETURN_STRING((char *)version, 0);
}
/* }}} */

/* {{{ proto void python_construct(string module, string class[, mixed ...])
 */
PHP_FUNCTION(python_construct)
{
	PHP_PYTHON_FETCH(pip, getThis());
	PyObject *module;
	char *module_name, *class_name;
	int module_name_len, class_name_len;

	/*
	 * We expect at least two parameters: the module name and the class name.
	 * Any additional parameters will be passed to the Python __init__ method
	 * down below.
	 */
	if (zend_parse_parameters(2 TSRMLS_CC, "ss",
							  &module_name, &module_name_len,
							  &class_name, &class_name_len) == FAILURE) {
		return;
	}

	PHP_PYTHON_THREAD_ACQUIRE();

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
			pip->object = PyObject_CallObject(class, args);
			if (args)
				Py_DECREF(args);

			if (pip->object == NULL)
				python_error(E_ERROR TSRMLS_CC);

			/* Our new object should be an instance of the requested class. */
			assert(PyObject_IsInstance(pip->object, class));
		} else
			php_error(E_ERROR, "Python: '%s.%s' is not a callable object",
					  module_name, class_name);

		Py_DECREF(module);
	} else
		php_error(E_ERROR, "Python: '%s' is not a valid module", module_name);

	PHP_PYTHON_THREAD_RELEASE();
}
/* }}} */
/* {{{ proto mixed python_eval(string expr)
   Evaluate a string of code by passing it to the Python interpreter. */
PHP_FUNCTION(python_eval)
{
	PyObject *m, *d, *v;
	char *expr;
	int len;

	if (zend_parse_parameters(1 TSRMLS_CC, "s", &expr, &len) == FAILURE) {
		return;
	}

	PHP_PYTHON_THREAD_ACQUIRE();

	/*
	 * The command will be evaluated in __main__'s context (for both
	 * globals and locals).  If __main__ doesn't already exist, it will be
	 * created.
	 */
	m = PyImport_AddModule("__main__");
	if (m == NULL) {
		PHP_PYTHON_THREAD_RELEASE();
		RETURN_NULL();
	}
	d = PyModule_GetDict(m);

	/*
	 * The string is evaluated as a single, isolated expression.  It is not
	 * treated as a statement or as series of statements.  This allows us to
	 * retrieve the resulting value of the evaluated expression.
	 */
	v = PyRun_String(expr, Py_eval_input, d, d);
	if (v == NULL) {
		python_error(E_WARNING TSRMLS_CC);
		PHP_PYTHON_THREAD_RELEASE();
		RETURN_NULL();
	}

	/*
	 * We convert the PyObject* value to a zval* so that we can return the
	 * result to PHP.  If the conversion fails, we'll still be left with a
	 * valid zval* equal to PHP's NULL.
	 *
	 * At this point, we're done with our PyObject* value, as well.  We can
	 * safely release our reference to it now.
	 */
	if (pip_pyobject_to_zval(v, return_value TSRMLS_CC) == FAILURE)
		ZVAL_NULL(return_value);

	Py_DECREF(v);

	PHP_PYTHON_THREAD_RELEASE();
}
/* }}} */
/* {{{ proto bool python_exec(string command)
   Execute a string of code by passing it to the Python interpreter. */
PHP_FUNCTION(python_exec)
{
	PyObject *m, *d, *v;
	char *command;
	int len;

	if (zend_parse_parameters(1 TSRMLS_CC, "s", &command, &len) == FAILURE) {
		return;
	}

	PHP_PYTHON_THREAD_ACQUIRE();

	/*
	 * The command will be evaluated in __main__'s context (for both
	 * globals and locals).  If __main__ doesn't already exist, it will be
	 * created.
	 */
	m = PyImport_AddModule("__main__");
	if (m == NULL) {
		PHP_PYTHON_THREAD_RELEASE();
		RETURN_NULL();
	}
	d = PyModule_GetDict(m);

	/*
	 * The string is executed as a sequence of statements.  This means that
	 * can only detect the overall success or failure of the execution.  We
	 * cannot return any other kind of result value.
	 */
	v = PyRun_String(command, Py_file_input, d, d);
	if (v == NULL) {
		python_error(E_WARNING TSRMLS_CC);
		PHP_PYTHON_THREAD_RELEASE();
		RETURN_FALSE;
	}

	Py_DECREF(v);

	PHP_PYTHON_THREAD_RELEASE();

	RETURN_TRUE;
}
/* }}} */
/* {{{ proto void python_call(string module, string function[, mixed ...])
   Call the requested function in the requested module. */
PHP_FUNCTION(python_call)
{
	char *module_name, *function_name;
	int module_name_len, function_name_len;
	PyObject *module;

	/* Parse only the first two parameters (module name and function name). */
	if (zend_parse_parameters(2 TSRMLS_CC, "ss", &module_name, &module_name_len,
							  &function_name, &function_name_len) == FAILURE) {
		return;
	}

	PHP_PYTHON_THREAD_ACQUIRE();

	/* Attempt to import the requested module. */
	module = PyImport_ImportModule(module_name);
	if (module) {
		PyObject *dict, *function;

		/* Look up the function name in the module's dictionary. */
		dict = PyModule_GetDict(module);
		function = PyDict_GetItemString(dict, function_name);
		if (function) {
			PyObject *args, *result;

			/*
			 * Call the function with a tuple of arguments.  We skip the first
			 * two arguments to python_call (the module name and function name)
			 * and pack the remaining values into the 'args' tuple.
			 */
			args = pip_args_to_tuple(ZEND_NUM_ARGS(), 2 TSRMLS_CC);
			result = PyObject_CallObject(function, args);
			if (args)
				Py_DECREF(args);

			if (result) {
				/* Convert the Python result to its PHP equivalent. */
				pip_pyobject_to_zval(result, return_value TSRMLS_CC);
				Py_DECREF(result);
			} else {
				python_error(E_ERROR TSRMLS_CC);
			}
		} else {
			python_error(E_ERROR TSRMLS_CC);
		}
		Py_DECREF(module);
	} else {
		python_error(E_ERROR TSRMLS_CC);
	}

	PHP_PYTHON_THREAD_RELEASE();
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
