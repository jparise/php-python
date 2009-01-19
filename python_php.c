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

#include "php.h"
#include "php_python_internal.h"

/* {{{ efree_array
 */
static void
efree_array(zval ***p, int n)
{
	int i;

	for (i = 0; i < n; ++i)
		efree(p[i]);

	efree(p);
}
/* }}} */

/* {{{ php_call
 */
static PyObject *
php_call(PyObject *self, PyObject *args)
{
	const char *name;
	int name_len, i, argc;
	zval ***argv, *lcname, *ret;
	PyObject *params;

	TSRMLS_FETCH();

	if (!PyArg_ParseTuple(args, "s#|O:call", &name, &name_len, &params))
		return NULL;

	/* Name entries in the PHP function_table are always lowercased. */
	MAKE_STD_ZVAL(lcname);
	ZVAL_STRINGL(lcname, (char *)name, name_len, 1);
	zend_str_tolower(Z_STRVAL_P(lcname), name_len);

	/* If this isn't a valid PHP function name, we cannot proceed. */
	if (!zend_hash_exists(CG(function_table), Z_STRVAL_P(lcname), name_len+1)) {
		PyErr_Format(PyExc_NameError, "Function does not exist: %s",
					 Z_STRVAL_P(lcname));
		return NULL;
	}

	/* Convert the parameters into PHP values. */
	argc = PyTuple_Size(params);
	argv = emalloc(sizeof(zval **) * argc);

	for (i = 0; i < argc; ++i) {
		PyObject *item = PyTuple_GetItem(params, i);

		argv[i] = emalloc(sizeof(zval *));
		MAKE_STD_ZVAL(*argv[i]);

		if (pip_pyobject_to_zval(item, *argv[i] TSRMLS_CC) != SUCCESS) {
			PyErr_Format(PyExc_ValueError, "Bad argument at index %d", i);
			efree_array(argv, argc);
			return NULL;
		}
	}

	/* Now we can call the PHP function. */
	if (call_user_function_ex(CG(function_table), (zval **)NULL, lcname, &ret,
							  argc, argv, 0, NULL TSRMLS_CC) != SUCCESS) {
		PyErr_Format(PyExc_Exception, "Failed to execute function: %s",
					 Z_STRVAL_P(lcname));
		efree_array(argv, argc);
		return NULL;
	}

	efree_array(argv, argc);

	return pip_zval_to_pyobject(ret TSRMLS_CC);
}
/* }}} */
/* {{{ php_var
 */
static PyObject *
php_var(PyObject *self, PyObject *args)
{
	char *name;
	int len;
	zval **v;

	TSRMLS_FETCH();

	if (!PyArg_ParseTuple(args, "s#", &name, &len))
		return NULL;

	if (zend_hash_find(&EG(symbol_table), name, len+1, (void **)&v) != SUCCESS) {
		PyErr_Format(PyExc_NameError, "Undefined variable: %s", name);
		return NULL;
	}

	return pip_zval_to_pyobject(*v TSRMLS_CC);
}
/* }}} */
/* {{{ php_version
 */
static PyObject *
php_version(PyObject *self, PyObject *args)
{
	return Py_BuildValue("s", PHP_VERSION);
}
/* }}} */

/* {{{ python_php_methods[]
 */
static PyMethodDef python_php_methods[] = {
	{"call",			php_call,			METH_VARARGS},
	{"var",				php_var,			METH_VARARGS},
	{"version",			php_version,		METH_NOARGS},
	{NULL, NULL, 0, NULL}
};
/* }}} */
/* {{{ int python_php_init()
 */
int
python_php_init()
{
	if (Py_InitModule3("php", python_php_methods, "PHP Module") == NULL)
		return FAILURE;

	return SUCCESS;
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
