--TEST--
Python: py_path, py_path_prepend, py_path_append
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

py_path_prepend('prepend');
$path = py_path();
echo $path[0] . "\n";

py_path_append('append');
$path = py_path();
echo $path[count($path) - 1] . "\n";
--EXPECT--
prepend
append
