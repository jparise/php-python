<?php
// This script prints "skip" if condition does not meet.
if (!extension_loaded("python") && ini_get("enable_dl")) {
  $dlext = (substr(PHP_OS, 0, 3) == "WIN") ? ".dll" : ".so";
  @dl("python$dlext");
}
if (!extension_loaded("python")) {
    die("skip\n");
}
?>
