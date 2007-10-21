--TEST--
Python: Object (dimension_delete)
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php
$py = python_eval('[1, 2, 3]');
var_dump(isset($py[2]));
unset($py[2]);
var_dump(isset($py[2]));
--EXPECT--
bool(true)
bool(false)
