--TEST--
Python: python_exec()
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$result = python_exec("print 'From Python: Hello PHP World'");
echo $result;
--EXPECT--
From Python: Hello PHP World
1
