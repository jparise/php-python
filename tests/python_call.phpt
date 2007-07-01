--TEST--
Python: python_call()
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

$py = <<<EOT
def Test(n):
    print "Testing"
    l = [1, 2, 3]
    l.append(n)
    return l
EOT;

# Execute the python code (in the __main__module)
var_dump(python_exec($py));
echo "\n";

# Execute it the simple way.
var_dump(python_exec('Test(4)'));
echo "\n";

# Execute it the more interesting way.
var_dump(python_call('__main__', 'Test', 4));
echo "\n";

--EXPECT--
bool(true)

Testing
bool(true)

Testing
array(4) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
}
