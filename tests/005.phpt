--TEST--
Python: py_import
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

py_import('math');
py_eval('print math.cos(0)');
--EXPECT--
1.0
