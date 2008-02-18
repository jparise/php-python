<?php

require_once 'PEAR/PackageFileManager2.php';
PEAR::setErrorHandling(PEAR_ERROR_DIE);

$desc = <<<EOT
This extension allows the Python interpreter to be embedded inside of PHP, allowing for the instantiate and manipulation of Python objects from within PHP.
EOT;

$version = '0.8.0';
$notes = <<<EOT
The extension has been largely rewritten to take advantage of the latest Zend object module.  Basic functionality is available but many features are still to come.  This release should be considered a "preview" release, intended to encourage general interest and feedback.

This release is designed to work with Python 2.5.  Other releases has not been tested.
EOT;

$package = new PEAR_PackageFileManager2();

$result = $package->setOptions(array(
    'filelistgenerator' => 'cvs',
    'changelogoldtonew' => false,
    'simpleoutput'		=> true,
    'baseinstalldir'    => '/',
    'packagefile'       => 'package.xml',
    'packagedirectory'  => '.',
    'clearcontents'     => true,
    'ignore'            => array('package.php', 'package.xml'),
    'dir_roles'         => array(
         'docs'                 => 'doc',
         'examples'             => 'doc',
         'tests'                => 'test',
    ),
    'exceptions'        => array(
         'CREDITS'              => 'doc',
         'EXPERIMENTAL'         => 'doc',
         'TODO'                 => 'doc',
         'Makefile.in'          => 'src',
    ),
));

if (PEAR::isError($result)) {
    echo $result->getMessage();
    die();
}

$package->clearDeps();
$package->setPackage('python');
$package->setPackageType('extsrc');
$package->setSummary('Embedded Python');
$package->setDescription($desc);
$package->setChannel('pecl.php.net');
$package->setLicense('MIT License');
$package->addMaintainer('lead', 'jon', 'Jon Parise', 'jon@php.net');

$package->addRelease();
$package->setProvidesExtension('python');
$package->setAPIVersion('0.6.0');
$package->setAPIStability('alpha');
$package->setReleaseVersion($version);
$package->setReleaseStability('alpha');
$package->setNotes($notes);
$package->setPhpDep('5.0.0');
$package->setPearInstallerDep('1.4.3');

$package->generateContents();

if ($_SERVER['argv'][1] == 'commit') {
    $result = $package->writePackageFile();
} else {
    $result = $package->debugPackageFile();
}

if (PEAR::isError($result)) {
    echo $result->getMessage();
    die();
}
