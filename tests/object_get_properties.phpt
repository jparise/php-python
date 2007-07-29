--TEST--
Python: Object (get_properties)
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
python_exec("
class Test:
	a = 1
	b = 2

	def __init__(self):
		self.c = 3

t = Test()
");
$vars = get_object_vars(python_eval('t'));
ksort($vars);
var_dump($vars);
--EXPECT--
array(6) {
  ["__doc__"]=>
  NULL
  ["__init__"]=>
  object(Python <type 'function'>)#2 (0) {
  }
  ["__module__"]=>
  string(8) "__main__"
  ["a"]=>
  int(1)
  ["b"]=>
  int(2)
  ["c"]=>
  int(3)
}
