--TEST--
Python: php module
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$php_value = "From PHP: Hello Python World";

$code = <<<EOD
import php

print php.var('php_value')
EOD;

py_eval($code);
--EXPECT--
From PHP: Hello Python World
