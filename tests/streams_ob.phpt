--TEST--
Python: Default streams
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php

$py = <<<EOT
print "Line 1"
print "Line 2"
print "Line 3"
EOT;

echo "Output Buffering - Start\n";
ob_start();
python_exec($py);
$output = ob_get_contents();
echo "Output Buffering - End\n";

echo "Captured Output\n";
echo $output;
--EXPECT--
Output Buffering - Start
Line 1
Line 2
Line 3
Output Buffering - End
Captured Output
Line 1
Line 2
Line 3
