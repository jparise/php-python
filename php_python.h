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

#ifndef PHP_PYTHON_H
#define PHP_PYTHON_H

extern zend_module_entry python_module_entry;
#define phpext_python_ptr &python_module_entry

#ifdef PHP_WIN32
#define PHP_PYTHON_API __declspec(dllexport)
#else
#define PHP_PYTHON_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(python);
PHP_MSHUTDOWN_FUNCTION(python);
PHP_RINIT_FUNCTION(python);
PHP_RSHUTDOWN_FUNCTION(python);
PHP_MINFO_FUNCTION(python);

#if 0
PHP_FUNCTION(py_path);
PHP_FUNCTION(py_path_prepend);
PHP_FUNCTION(py_path_append);
PHP_FUNCTION(py_import);
PHP_FUNCTION(py_eval);
PHP_FUNCTION(py_call);
#endif

ZEND_BEGIN_MODULE_GLOBALS(python)
	zend_bool dummy;
ZEND_END_MODULE_GLOBALS(python)

#ifdef ZTS
#define PYG(v) TSRMG(python_globals_id, zend_python_globals *, v)
#else
#define PYG(v) (python_globals.v)
#endif

#endif /* PHP_PYTHON_H */
