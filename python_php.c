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
	{"version",			php_version,		METH_NOARGS},
	{"var",				php_var,			METH_VARARGS},
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
