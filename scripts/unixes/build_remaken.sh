#!/bin/bash
QTVERSION=5.15.2
PROJECTROOT=../..

# default linux values
QMAKEPATH=$HOME/Qt/${QTVERSION}/gcc_64/bin
if [[ "$OSTYPE" == "darwin"* ]]; then
	# overload for mac values
	QMAKE_PATH=/Application/Qt/${QTVERSION}/clang_64/bin
fi

display_usage() { 
	echo "This script builds Remaken in shared mode."
    echo "It can receive three optional arguments." 
	echo -e "\nUsage: \$0 [path to xpcf project root | default='${PROJECTROOT}'] [Qt kit version to use | default='${QTVERSION}'] [path to qmake | default='${QMAKEPATH}'] \n" 
} 

# check whether user had supplied -h or --help . If yes display usage 
if [[ ( $1 == "--help") ||  $1 == "-h" ]] 
then 
    display_usage
    exit 0
fi 

if [ $# -ge 1 ]; then
	PROJECTROOT=$1
fi
if [ $# -ge 2 ]; then
	QTVERSION=$2
fi
if [ $# -eq 3 ]; then
	QMAKEPATH=$3
fi

${PROJECTROOT}/scripts/unixes/build_remaken_project.sh remaken shared ${PROJECTROOT} ${QTVERSION} ${QMAKEPATH}
