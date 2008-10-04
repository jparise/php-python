--TEST--
Python: Default streams
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php

function errorHandler($errno, $errstr, $errfile, $errline)
{
    echo "Error $errno: $errstr\n";
}

set_error_handler('errorHandler');

$py = <<<EOT
import sys
print >> sys.stdout, 'sys.stdout'
print >> sys.stderr, 'sys.stderr'
EOT;

python_exec($py);
--EXPECT--
sys.stdout
Error 8: sys.stderr
Error 8:
