--TEST--
Python: php.call()
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php

$py = <<<EOT
import php

print php.call('sha1', ('apple',))
EOT;

python_exec($py);
--EXPECT--
d0be2dc421be4fcd0172e5afceea3970e2f3d940
