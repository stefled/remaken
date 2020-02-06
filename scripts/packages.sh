#!/bin/bash

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    echo "installing cmake\n"
    sudo apt install cmake
    echo "installing conan\n"
    sudo apt-get install  python-pip
    sudo pip install conan
    # Mandatory Set in your profile : compiler.libcxx=libstdc++11
    echo "Updating conan setting : compiler.libcxx=libstdc++11\n"
    conan profile update settings.compiler.libcxx="libstdc++11" default
    echo "installing zlib1g-dev\n"
    sudo apt-get install zlib1g-dev 
    # zipper build : to replace with conan package later
    echo "building zipper compression c++ wrapper\n"
    git clone --recursive https://github.com/sebastiandev/zipper.git
    cd zipper
    mkdir build
    cd build
    cmake ..
    make
    echo "installing zipper compression c++ wrapper\n"
    sudo make install
    cd -
        # ...
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    if test ! $(which brew); then
	echo "brew is not installed, first install brew then relaunch this script\n";
	exit 1
    fi
    echo "installing openssl brew\n"
    brew install openssl
    #next line is mandatory for configure to find the include and libraries files from the command line
    #NOTE : once compilation is done, you should unlink with brew unlink openssl to avoid conflict in macosX
    echo "Before compilation, do \nbrew link --force openssl\nOnce compilation is finalized do\nbrew unlink openssl\n to avoid conflict with mac OS X openssl version\n"
    #You must add a libjpeg.pc file such as the one provided as a sample with this script in /usr/local/lib/pkgconfig
#elif [[ "$OSTYPE" == "cygwin" ]]; then
        # POSIX compatibility layer and Linux environment emulation for Windows
#elif [[ "$OSTYPE" == "msys" ]]; then
        # Lightweight shell and GNU utilities compiled for Windows (part of MinGW)
#elif [[ "$OSTYPE" == "win32" ]]; then
        # I'm not sure this can happen.
#elif [[ "$OSTYPE" == "freebsd"* ]]; then
        # ...
#else
        # Unknown.
fi

