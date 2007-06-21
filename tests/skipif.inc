<?php
/*
 * This helper script will print "skip" if the Python module cannot be made
 * available to the tests' executable environment.
 */

if (!extension_loaded("python") && ini_get("enable_dl")) {
  $dlext = (substr(PHP_OS, 0, 3) == "WIN") ? ".dll" : ".so";
  @dl("python$dlext");
}
if (!extension_loaded("python")) {
    die("skip\n");
}
?>
