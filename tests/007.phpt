--TEST--
Python: python.prepend_path and python_append_path
--SKIPIF--
<?php include('skipif.inc'); ?>
--INI--
python.prepend_path=/prepend1:/prepend2
python.append_path=/append1;/append2
--FILE--
<?php

$path = py_path();
var_dump(array_slice($path, 0, 2));
var_dump(array_slice($path, -2, 2));
--EXPECT--
array(2) {
  [0]=>
  string(9) "/prepend2"
  [1]=>
  string(9) "/prepend1"
}
array(2) {
  [0]=>
  string(8) "/append1"
  [1]=>
  string(8) "/append2"
}
