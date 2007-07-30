--TEST--
Python: Object (dimension_delete)
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$py = python_eval('(1, 2, 3)');
var_dump(isset($py[1]));
unset($py[1]);
var_dump(isset($py[1]));
--EXPECT--
bool(true)
bool(false)
