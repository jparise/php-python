<?php

echo "Python in PHP (" . PHP_PYTHON_VERSION . ")\n";
echo python_version() . "\n";

$stdin = fopen('php://stdin', 'r');

while (!feof($stdin)) {
    echo ">>> ";
    $line = rtrim(fgets($stdin));
    python_exec($line);
}
