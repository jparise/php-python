--TEST--
Python: Python to PHP type conversions
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

py_path_prepend(dirname(__FILE__));

$p = new Python('TestModule', 'TestClass');
echo $p->returnInt() . "\n";
echo $p->returnString() . "\n";

print_r($p->returnTuple()) . "\n";
print_r($p->returnList()) . "\n";
print_r($p->returnDict()) . "\n";

var_dump($p->returnSelf()) . "\n";
var_dump($p->returnNone()) . "\n";
--EXPECT--
1113
Test String
Array
(
    [0] => 1
    [1] => two
    [2] => 3
)
Array
(
    [0] => 1
    [1] => two
    [2] => 3
)
Array
(
    [three] => 3
    [two] => 2
    [one] => 1
)
object(Python)(1) {
  [0]=>
  resource(5) of type (Python)
}
NULL
