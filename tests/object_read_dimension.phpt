--TEST--
Python: Object (read_dimension)
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$py = python_eval('(1, 2, 3)');
var_dump($py[1]);
--EXPECT--
int(2)
