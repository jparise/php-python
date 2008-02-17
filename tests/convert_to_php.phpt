--TEST--
Python: Convert Python types to PHP types
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php
var_dump(python_eval("None"));
var_dump(python_eval("int(1)"));
var_dump(python_eval("long(1)"));
var_dump(python_eval("1.5"));

var_dump(python_eval("'string'"));
var_dump(python_eval("r'string'"));
var_dump(python_eval("u'string'"));

var_dump(python_eval("(1, 2, 3)"));
var_dump(python_eval("[1, 2, 3]"));
var_dump(python_eval("{'one': 1, 'two': 2, 'three': 3}"));
--EXPECT--
NULL
int(1)
int(1)
float(1.5)
string(6) "string"
string(6) "string"
string(6) "string"
object(Python <type 'tuple'>)#1 (3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
object(Python <type 'list'>)#1 (3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
object(Python <type 'dict'>)#1 (3) {
  ["three"]=>
  int(3)
  ["two"]=>
  int(2)
  ["one"]=>
  int(1)
}
