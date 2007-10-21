--TEST--
Python: python_eval()
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php
echo python_eval("10 * 10");
--EXPECT--
100
