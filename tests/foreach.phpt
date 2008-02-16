--TEST--
Python: foreach
--SKIPIF--
<?php if (!extension_loaded('python')) die("skip\n");
--FILE--
<?php

function dump($a)
{
    ksort($a, SORT_REGULAR);
    foreach ($a as $k => $v) {
        echo "$k -> $v\n";
    }
}

echo "Tuple\n";
python_exec("a = (1, 2, 3)");
dump(python_eval('a'));
echo "\n";

echo "List\n";
python_exec("a = [1, 2, 3]");
dump(python_eval('a'));
echo "\n";

echo "Dictionary\n";
python_exec("a = {'one': 1, 'two': 2, 'three': 3}");
dump(python_eval('a'));

--EXPECT--
Tuple
0 -> 1
1 -> 2
2 -> 3

List
0 -> 1
1 -> 2
2 -> 3

Dictionary
one -> 1
three -> 3
two -> 2
