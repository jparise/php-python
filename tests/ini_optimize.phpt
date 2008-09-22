--TEST--
Python: INI python.optimize
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--INI--
python.optimize=1
--FILE--
<?php
echo python_eval("__debug__");
--EXPECT--
0
