--TEST--
Python: Object (write_dimension)
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$py = python_eval('(1, 2, 3)');
$py[1] = 4;
var_dump($py[1]);
--EXPECT--
int(4)
