#!/bin/bash

wget https://github.com/b-com-software-basis/remaken/releases/download/1.10.0%2Funixes/remaken-ubuntu22.04

sudo chmod +x remaken-ubuntu22.04
sudo mv remaken-ubuntu22.04 /usr/local/bin/remaken

# install cmake
sudo apt-get -y install cmake unzip
# install pkg-config
sudo apt install pkg-config

# remaken init
remaken init

