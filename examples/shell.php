<?php

echo 'Python in PHP (' . python_version() . ")\n";

$stdin = fopen('php://stdin', 'r');

while (!feof($stdin)) {
    echo ">>> ";
    $line = rtrim(fgets($stdin));
    python_exec($line);
}
