#!/bin/bash

brew tap b-com/sft
if [ "$#" -gt 0 ]; then
   echo "install remaken version $1"
   brew install remaken@$1
else
   echo "install latest version of remaken"
   brew install remaken
fi

remaken init
