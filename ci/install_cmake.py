#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import platform
import urllib
import zipfile
import os
import shutil

CMAKE_BASE_URL = "https://cmake.org/files/v3.5/"
CMAKE_FILENAME_LINUX_32 = "cmake-3.5.0-rc3-Linux-i386"
CMAKE_FILENAME_LINUX_64 = "cmake-3.5.0-rc3-Linux-x86_64"
CMAKE_FILENAME_WINDOWS = "cmake-3.5.0-rc3-win32-x86"
CMAKE_FILENAME_MACOSX = "cmake-3.5.0-rc3-Darwin-x86_64"
CMAKE_SUFFIX_UNIX = ".tar.gz"
CMAKE_SUFFIX_WINDOWS = ".zip"
CMAKE_DEST = "ci/build"

def report(blocknr, blocksize, size):
    current = blocknr*blocksize
    sys.stdout.write("\rDownloading "+CMAKE_FILENAME+CMAKE_SUFFIX+" "+"{0:.2f}%".format(100.0*current/size))

def download(url):
    filename = url.split("/")[-1]
    urllib.urlretrieve(url, filename, report)
    sys.stdout.write("\n")

def extract(filename):
    sys.stdout.write("Extracting "+filename)
    file = zipfile.ZipFile(filename)
    file.extractall()
    sys.stdout.write("\n")

def copy(source, dest):
    if os.path.exists(dest):
        shutil.rmtree(dest)
    sys.stdout.write("Copying to "+dest)
    shutil.copytree(source, dest)
    sys.stdout.write("\n")

if __name__ == "__main__":
    if not os.path.exists(CMAKE_DEST):
        os.mkdir(CMAKE_DEST)
    os.chdir(CMAKE_DEST)
    if platform.system() == "Linux":
        if platform.architecture()[0] == "32bit":
            CMAKE_FILENAME = CMAKE_FILENAME_LINUX_32
        elif platform.architecture()[0] == "64bit":
            CMAKE_FILENAME = CMAKE_FILENAME_LINUX_64   
        CMAKE_SUFFIX = CMAKE_SUFFIX_UNIX
    elif platform.system() == "Windows":
        CMAKE_FILENAME = CMAKE_FILENAME_WINDOWS
        CMAKE_SUFFIX = CMAKE_SUFFIX_WINDOWS
    elif platform.system() == "Darwin":
        CMAKE_FILENAME = CMAKE_FILENAME_MACOSX
        CMAKE_SUFFIX = CMAKE_SUFFIX_UNIX

    download(CMAKE_BASE_URL+CMAKE_FILENAME+CMAKE_SUFFIX)
    extract(CMAKE_FILENAME+CMAKE_SUFFIX)
    copy(CMAKE_FILENAME, "cmake")

    print "Please prepend "+CMAKE_DEST+"/cmake/bin"+" to PATH"