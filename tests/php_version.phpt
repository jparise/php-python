--TEST--
Python: php.version()
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php

$py = <<<EOT
import php
version = php.version()
EOT;

# Verify that php.version() returns the same string as PHP_VERSION.
python_exec($py);
var_dump(python_eval('version') == PHP_VERSION);

--EXPECT--
bool(true)
