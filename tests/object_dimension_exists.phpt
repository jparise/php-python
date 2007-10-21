--TEST--
Python: Object (dimension_exists)
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php
$py = python_eval('(1, 2, 3)');
var_dump(isset($py[1]));
var_dump(isset($py[4]));
--EXPECT--
bool(true)
bool(false)
