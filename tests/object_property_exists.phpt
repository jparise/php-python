--TEST--
Python: Object (property_exists)
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
python_exec("
class Test:
	a = 1
");

$py = new Python('__main__', 'Test');
var_dump(isset($py->a));
var_dump(isset($py->b));
--EXPECT--
bool(true)
bool(false)
