--TEST--
Python: python_exec()
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

function test_exec($s)
{
	echo "Command: $s\n";
	$result = @python_exec($s);
	echo "Result:  " . (($result) ? "success\n" : "failure\n");
	echo "\n";
}

test_exec('i = 1');
test_exec('iiiii');
--EXPECT--
Command: i = 1
Result:  success

Command: iiiii
Result:  failure
