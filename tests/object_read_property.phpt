--TEST--
Python: Object (read_property)
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php
python_exec("
class Test:
	a = 1
");

$py = new Python('__main__', 'Test');
var_dump($py->a);
--EXPECT--
int(1)
