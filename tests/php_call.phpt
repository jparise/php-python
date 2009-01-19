--TEST--
Python: php.call()
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php

function test()
{
	return 'Test';
}

$py = <<<EOT
import php

print php.call('test')
print php.call('sha1', ('tuple',))
print php.call('sha1', ['list'])
EOT;

python_exec($py);
--EXPECT--
Test
49f80ea5aacfb5a957337dc906635fccbde446fc
38b62be4bddaa5661c7d6b8e36e28159314df5c7
