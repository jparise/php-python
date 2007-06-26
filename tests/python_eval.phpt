--TEST--
Python: python_eval()
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
echo python_eval("10 * 10");
--EXPECT--
100
