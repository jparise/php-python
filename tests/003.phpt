--TEST--
Python: PHP to Python type conversions
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$string = "test";
$bool = true;
$int = 50;
$float = 60.4;
$null = null;
$hash = array('a' => '1', 5 => 2, 'c' => 3);

$code = <<<EOD
import php

print php.var('string')
print php.var('bool')
print php.var('int')
print php.var('float')
print php.var('null')
print php.var('hash')
EOD;

py_eval($code);
--EXPECT--
test
1
50
60.4
None
{'a': '1', 'c': 3, 5: 2}
