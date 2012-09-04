# Python in PHP

Copyright &copy; 2002-2012 Jon Parise

## Overview

This code is released under the terms of the [MIT License][license].

### Requirements

The Python extension has the following system requirements:

- PHP version 5.2.0 or later
- PEAR installer version 1.4.3 or later

### Installation

The easiest way to install the Perforce extension is by using the PECL
installer::

    pecl install python

If you're building from source code, should use the ``--with-python``
configuration option.  If your copy of the Python hasn't been installed in one
of the default locations, you can specify it as a parameter::

    --with-python=/opt/python

More detailed information on installing PECL packages in general can be found
in the [Installation of PECL Extensions][pecl-install] section of the [PHP
Manual][php-manual].

### Source Code

This package's source code is hosted on GitHub:

    https://github.com/jparise/php-python.git

A number of unit tests are included to help maintain correct and expected
behavior.  The tests can be run using PECL tool's `run-tests` command:

    pecl run-tests -p python

Additional unit test contributions are welcome, especially when they increase
code coverage or exercise specific use cases.

## Usage

### INI Settings

#### python.optimize

The `python.optimize` INI setting controls the Python interpreter's runtime
optimization level.  This is a global, system-wide setting and therefore can
only be set in the PHP.ini file.

When set to **0** (the default), no additional optimizations are enabled, and
the built-in `__debug__` flag is True.

When set to **1** (equivalent to Python's `-O` command line option), the
Python interpreter will generate optimized bytecode, and the built-in
`__debug__` flag is set to False.

When set to **2** (equivalent to Python's `-OO` command line option), the
Python doc-strings will be removed in addition to the above optimizations.

## Development and Support

### Reporting Problems and Suggestions

If you run into a problem or would like to make a suggestion, please use the
[issue tracker][].  Feel free to contact me directly for other issues, but
please try to use the bug tracker whenever possible so that others in the
community will benefit from your feedback and my responses.

[license]: http://www.opensource.org/licenses/mit-license.php
[pecl-install]: http://www.php.net/manual/install.pecl.php
[php-manual]: http://www.php.net/manual/
[issue tracker]: https://github.com/jparise/php-python/issues
