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

/* {{{ OutputStream
 */
typedef struct {
	PyObject_HEAD
} OutputStream;

static PyTypeObject OutputStream_Type;

/* {{{ OutputStream_close
 */
static PyObject *
OutputStream_close(OutputStream *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":close"))
		return NULL;

	PyErr_SetString(PyExc_RuntimeError, "Output stream cannot be closed");
	return NULL;
}
/* }}} */
/* {{{ OutputStream_flush
 */
static PyObject *
OutputStream_flush(OutputStream *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":flush"))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}
/* }}} */
/* {{{ OutputStream_write
 */
static PyObject *
OutputStream_write(OutputStream *self, PyObject *args)
{
	const char *str;
	int len;

	if (!PyArg_ParseTuple(args, "s#:write", &str, &len))
		return NULL;

	ZEND_WRITE(str, len);

	Py_INCREF(Py_None);
	return Py_None;
}
/* }}} */
/* {{{ OutputStream_writelines
 */
static PyObject *
OutputStream_writelines(OutputStream *self, PyObject *args)
{
	PyObject *sequence;
	PyObject *iterator;
	PyObject *item;
	char *str;
	int len;

	if (!PyArg_ParseTuple(args, "O:writelines", &sequence))
        return NULL;

	iterator = PyObject_GetIter(sequence);
	if (iterator == NULL)
		return NULL;

	while (item = PyIter_Next(iterator)) {
		if (PyString_AsStringAndSize(item, &str, &len) != -1) {
			ZEND_WRITE(str, len);
			Py_DECREF(item);
		} else {
			Py_DECREF(item);
			break;
		}
	}

	Py_DECREF(iterator);

	if (item && !str)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}
/* }}} */

/* {{{ OutputStream_closed
 */
static PyObject *
OutputStream_closed(OutputStream *self, void *closure)
{
	Py_INCREF(Py_False);
	return Py_False;
}
/* }}} */
/* {{{ OutputStream_isatty
 */
static PyObject *
OutputStream_isatty(OutputStream *self, void *closure)
{
	Py_INCREF(Py_False);
	return Py_False;
}
/* }}} */

/* {{{ OutputStream_methods
 */
static PyMethodDef OutputStream_methods[] = {
	{ "close",		(PyCFunction)OutputStream_close,		METH_VARARGS, 0 },
	{ "flush",		(PyCFunction)OutputStream_flush,		METH_VARARGS, 0 },
	{ "write",		(PyCFunction)OutputStream_write,		METH_VARARGS, 0 },
	{ "writelines",	(PyCFunction)OutputStream_writelines,	METH_VARARGS, 0 },
	{ NULL, NULL}
};
/* }}} */
/* {{{ OutputStream_getset
 */
static PyGetSetDef OutputStream_getset[] = {
	{ "closed",		(getter)OutputStream_closed,	NULL, 0 },
	{ "isatty",		(getter)OutputStream_isatty,	NULL, 0 },
	{ NULL },
};
/* }}} */
/* {{{ OutputStream_Type
 */
static PyTypeObject OutputStream_Type = {
	PyObject_HEAD_INIT(NULL)
	0,													/* ob_size */
	"php.OutputStream",									/* tp_name */
	sizeof(OutputStream),								/* tp_basicsize */
	0,													/* tp_itemsize */
	0,													/* tp_dealloc */
	0,													/* tp_print */
	0,													/* tp_getattr */
	0,													/* tp_setattr */
	0,													/* tp_compare */
	0,													/* tp_repr */
	0,													/* tp_as_number */
	0,													/* tp_as_sequence */
	0,													/* tp_as_mapping */
	0,													/* tp_hash */
	0,													/* tp_call */
	0,													/* tp_str */
	0,													/* tp_getattro */
	0,													/* tp_setattro */
	0,													/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,									/* tp_flags */
	"PHP OutputStream",									/* tp_doc */
	0,													/* tp_traverse */
	0,													/* tp_clear */
	0,													/* tp_richcompare */
	0,													/* tp_weaklistoffset */
	0,													/* tp_iter */
	0,													/* tp_iternext */
	OutputStream_methods,								/* tp_methods */
	0,													/* tp_members */
	OutputStream_getset,								/* tp_getset */
};
/* }}} */
/* }}} */
/* {{{ ErrorStream
 */
typedef struct {
	PyObject_HEAD
} ErrorStream;

static PyTypeObject ErrorStream_Type;

/* {{{ ErrorStream_close
 */
static PyObject *
ErrorStream_close(ErrorStream *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":close"))
		return NULL;

	PyErr_SetString(PyExc_RuntimeError, "Error stream cannot be closed");
	return NULL;
}
/* }}} */
/* {{{ ErrorStream_flush
 */
static PyObject *
ErrorStream_flush(ErrorStream *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":flush"))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}
/* }}} */
/* {{{ ErrorStream_write
 */
static PyObject *
ErrorStream_write(ErrorStream *self, PyObject *args)
{
	const char *str;

	if (!PyArg_ParseTuple(args, "s:write", &str))
		return NULL;

	php_error(E_NOTICE, "%s", str);

	Py_INCREF(Py_None);
	return Py_None;
}
/* }}} */
/* {{{ ErrorStream_writelines
 */
static PyObject *
ErrorStream_writelines(ErrorStream *self, PyObject *args)
{
	PyObject *sequence;
	PyObject *iterator;
	PyObject *item;
	const char *str;

	if (!PyArg_ParseTuple(args, "O:writelines", &sequence))
        return NULL;

	iterator = PyObject_GetIter(sequence);
	if (iterator == NULL)
		return NULL;

	while (item = PyIter_Next(iterator)) {
		if (str = PyString_AsString(item)) {
			php_error(E_NOTICE, "%s", str);
			Py_DECREF(item);
		} else {
			Py_DECREF(item);
			break;
		}
	}

	Py_DECREF(iterator);

	if (item && !str)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}
/* }}} */

/* {{{ ErrorStream_close
 */
static PyObject *
ErrorStream_closed(ErrorStream *self, void *closure)
{
	Py_INCREF(Py_False);
	return Py_False;
}
/* }}} */
/* {{{ ErrorStream_isatty
 */
static PyObject *
ErrorStream_isatty(ErrorStream *self, void *closure)
{
	Py_INCREF(Py_False);
	return Py_False;
}
/* }}} */

/* {{{ ErrorStream_methods
 */
static PyMethodDef ErrorStream_methods[] = {
	{ "close",		(PyCFunction)ErrorStream_close,			METH_VARARGS, 0 },
	{ "flush",		(PyCFunction)ErrorStream_flush,			METH_VARARGS, 0 },
	{ "write",		(PyCFunction)ErrorStream_write,			METH_VARARGS, 0 },
	{ "writelines",	(PyCFunction)ErrorStream_writelines,	METH_VARARGS, 0 },
	{ NULL, NULL}
};
/* }}} */
/* {{{ ErrorStream_getset
 */
static PyGetSetDef ErrorStream_getset[] = {
	{ "closed",		(getter)ErrorStream_closed,		NULL, 0 },
	{ "isatty",		(getter)ErrorStream_isatty,		NULL, 0 },
	{ NULL },
};
/* }}} */
/* {{{ ErrorStream_Type
 */
static PyTypeObject ErrorStream_Type = {
	PyObject_HEAD_INIT(NULL)
	0,													/* ob_size */
	"php.ErrorStream",									/* tp_name */
	sizeof(ErrorStream),								/* tp_basicsize */
	0,													/* tp_itemsize */
	0,													/* tp_dealloc */
	0,													/* tp_print */
	0,													/* tp_getattr */
	0,													/* tp_setattr */
	0,													/* tp_compare */
	0,													/* tp_repr */
	0,													/* tp_as_number */
	0,													/* tp_as_sequence */
	0,													/* tp_as_mapping */
	0,													/* tp_hash */
	0,													/* tp_call */
	0,													/* tp_str */
	0,													/* tp_getattro */
	0,													/* tp_setattro */
	0,													/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,									/* tp_flags */
	"PHP ErrorStream",									/* tp_doc */
	0,													/* tp_traverse */
	0,													/* tp_clear */
	0,													/* tp_richcompare */
	0,													/* tp_weaklistoffset */
	0,													/* tp_iter */
	0,													/* tp_iternext */
	ErrorStream_methods,								/* tp_methods */
	0,													/* tp_members */
	ErrorStream_getset,									/* tp_getset */
};
/* }}} */
/* }}} */

/* {{{ int python_streams_init()
   Initialize the Python streams interface. */
int
python_streams_init()
{
	if (PyType_Ready(&OutputStream_Type) == -1)
		return FAILURE;

	if (PyType_Ready(&ErrorStream_Type) == -1)
		return FAILURE;

	return SUCCESS;
}
/* }}} */
/* {{{ int python_streams_intercept()
   Redirect Python's streams to PHP equivalents. */
int
python_streams_intercept()
{
	PyObject *stream;

	/* Redirect sys.stdout to an instance of our output stream type. */
	stream = (PyObject *)PyObject_New(OutputStream, &OutputStream_Type);
	PySys_SetObject("stdout", stream);
	Py_DECREF(stream);

	/* Redirect sys.stderr to an instance of our error stream type. */
	stream = (PyObject *)PyObject_New(ErrorStream, &ErrorStream_Type);
	PySys_SetObject("stderr", stream);
	Py_DECREF(stream);

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
