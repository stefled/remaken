#!/bin/bash

brew tap b-com/sft

# install conan 1.59.0
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=TRUE
wget https://raw.githubusercontent.com/Homebrew/homebrew-core/1fc9e09956a2aa5ee2f2f4f6c81e359c232dee0f/Formula/conan.rb
brew install -f ./conan.rb
rm -rf ./conan.rb

# manage default remote
conan remote add --force --insert 0 conancenter https://center.conan.io

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

