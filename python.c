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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_python.h"
#include "php_python_internal.h"

ZEND_DECLARE_MODULE_GLOBALS(python)

static zend_class_entry python_class_entry;

/* {{{ python_functions[]
 */
function_entry python_functions[] = {
#if 0
	PHP_FE(py_path,			NULL)
	PHP_FE(py_path_prepend,	NULL)
	PHP_FE(py_path_append,	NULL)
	PHP_FE(py_import,		NULL)
	PHP_FE(py_eval,			NULL)
	PHP_FE(py_call,			NULL)
#endif
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
	NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PYTHON
ZEND_GET_MODULE(python)
#endif
/* }}} */
/* {{{ php_python_constructor_function
 */
zend_internal_function php_python_constructor_function = {
	ZEND_INTERNAL_FUNCTION,		/* type */
	"__construct",				/* function_name */
	&python_class_entry,		/* scope */
	0,							/* fn_flags */
	NULL,						/* prototype */
	2,							/* num_args */
	NULL,						/* arg_info */
	0,							/* pass_rest_by_reference */
	ZEND_FN(python_new)			/* handler */
};
/* }}} */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
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
	ZEND_INIT_MODULE_GLOBALS(python, python_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	INIT_CLASS_ENTRY(python_class_entry, "python", NULL);
	python_class_entry.create_object = python_object_create;
	python_class_entry.constructor = (zend_function *)&php_python_constructor_function;
	zend_register_internal_class(&python_class_entry TSRMLS_CC);

	/* Initialize the Python interpreter */
	Py_IgnoreEnvironmentFlag = 0;
	Py_Initialize();

	return Py_IsInitialized() ? SUCCESS : FAILURE;
}
/* }}} */
/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(python)
{
	UNREGISTER_INI_ENTRIES();

	/* Shut down the Python interpretter. */
	Py_Finalize();

	return SUCCESS;
}
/* }}} */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(python)
{
	return SUCCESS;
}
/* }}} */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(python)
{
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
	php_info_print_table_header(1, "Python Environment");
	php_info_print_table_row(2, "Module Search Path", Py_GetPath());
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(1, "Python Copyright");
	php_info_print_box_start(0);
	php_printf("%s", Py_GetCopyright());
	php_info_print_box_end();
	php_info_print_table_end();
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
