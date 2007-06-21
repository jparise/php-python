--TEST--
Python: python_eval()
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$result = python_eval("print 'From Python: Hello PHP World'");
echo $result;
--EXPECT--
From Python: Hello PHP World
1
