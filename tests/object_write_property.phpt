--TEST--
Python: Object (write_property)
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
python_exec("
class Test:
	a = 1
");

$py = new Python('__main__', 'Test');
var_dump($py->a);
$py->a = 2;
var_dump($py->a);
--EXPECT--
int(1)
int(2)
