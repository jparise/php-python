--TEST--
Python: Object (property_exists)
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
python_exec("
class Test:
    def __init__(self):
	    self.a = 1
");

$py = new Python('__main__', 'Test');
var_dump(isset($py->a));
unset($py->a);
var_dump(isset($py->a));
--EXPECT--
bool(true)
bool(false)
