--TEST--
Python: php.var()
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php

$global = 'global';

$py = <<<EOT
import php

var = php.var('global')

unknown = ''
try:
    php.var('unknown')
except NameError, e:
    unknown = str(e)
EOT;

python_exec($py);
var_dump(python_eval('var'));
var_dump(python_eval('unknown'));
--EXPECT--
string(6) "global"
string(27) "Undefined variable: unknown"
