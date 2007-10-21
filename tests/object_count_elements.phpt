--TEST--
Python: Object (count_elements)
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php
$py = python_eval('(1, 2, 3)');
var_dump(count($py));
--EXPECT--
int(3)
