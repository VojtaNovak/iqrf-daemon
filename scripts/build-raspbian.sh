#!/bin/bash
# Script for building IQRF daemon on Linux machine
# Tested on Raspberry PI 3, Raspbian Lite
# External dependencies: IBM Paho, Oracle Java

set -e

LIB_DIRECTORY=${1:-../..}
DAEMON_DIRECTORY=$PWD/../daemon
IQRFAPP_DIRECTORY=$PWD/../applications/iqrfapp
UTILS_DIRECTORY=cutils
LIBDPA_DIRECTORY=clibdpa
LIBCDC_DIRECTORY=clibcdc
LIBSPI_DIRECTORY=clibspi
PAHO_DIRECTORY=paho.mqtt.c

export JAVA_HOME=/usr/lib/jvm/jdk-8-oracle-arm32-vfp-hflt
export JAVA_INCLUDE_PATH=${JAVA_HOME}/include
export JAVA_INCLUDE_PATH2=${JAVA_INCLUDE_PATH}/linux

bash download-dependencies.sh ${LIB_DIRECTORY}

cd ${LIB_DIRECTORY}

# building paho
if [ -d "${PAHO_DIRECTORY}" ]; then
	echo "Building paho ..."
	cd ${PAHO_DIRECTORY}
	sudo apt-get install build-essential gcc make cmake libssl-dev
	cmake -G "Unix Makefiles" -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=TRUE .
	make
	sudo make install
	sudo ldconfig
	cd ..
fi

# building libspi
if [ -d "${LIBSPI_DIRECTORY}" ]; then
	echo "Building libspi ..."
	cd ${LIBSPI_DIRECTORY}
	sudo apt-get install oracle-java8-jdk
	bash buildMake.sh
	cd ..
fi

# building clibcdc, cutils, clibdpa
for repository in ${LIBCDC_DIRECTORY} ${UTILS_DIRECTORY} ${LIBDPA_DIRECTORY}; do
	if [ -d "${repository}" ]; then
		echo "Building ${repository} ..."
		cd ${repository}
		bash buildMake.sh
		cd ..
	fi
done

# building daemon
echo "Building daemon ..."
cd ${DAEMON_DIRECTORY}
bash buildMake.sh ${LIB_DIRECTORY}

# building iqrfapp
echo "Building iqrfapp ..."
cd ${IQRFAPP_DIRECTORY}
bash buildMake.sh ../${LIB_DIRECTORY}

