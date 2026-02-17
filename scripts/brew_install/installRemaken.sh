#!/bin/bash

brew tap b-com/sft

# install cmake
brew install -f cmake

if [ "$#" -gt 0 ]; then
   echo "install remaken version $1"
   brew install --ignore-dependencies remaken@$1
else
   echo "install latest version of remaken"
   brew install --ignore-dependencies remaken
fi

sudo apt install pkg-config

remaken init

