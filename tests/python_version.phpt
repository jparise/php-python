--TEST--
Python: python_version()
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php
echo python_version();
--EXPECTREGEX--
\d\.\d\.\d \(.*\)\s+\[.*\]
