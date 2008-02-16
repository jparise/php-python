<?php

echo 'Python in PHP (PHP ' . PHP_VERSION . ')' . "\n";

$stdin = fopen('php://stdin', 'r');

while (!feof($stdin)) {
    echo ">>> ";
    $line = rtrim(fgets($stdin));
    python_exec($line);
}
