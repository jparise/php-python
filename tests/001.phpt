--TEST--
Python: py_eval()
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
py_eval("print 'From Python: Hello PHP World'");
--EXPECT--
From Python: Hello PHP World
