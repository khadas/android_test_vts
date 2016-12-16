#!/bin/bash
# Only tested on linux so far
echo "Install Python SDK"
sudo apt-get install python-dev

echo "Install Protocol Buffer packages"
sudo apt-get install python-protobuf
sudo apt-get install protobuf-compiler

echo "Install Python virtualenv and pip tools for VTS TradeFed and Runner"
sudo apt-get install python-setuptools
sudo apt-get install python-pip
sudo apt-get install python3-pip
sudo apt-get install python-virtualenv

echo "Install Python modules for VTS Runner"
sudo pip install future
sudo pip install futures
sudo pip install enum
sudo pip install concurrent
sudo pip install protobuf
sudo pip install setuptools
sudo pip install requests

echo "Install packages for Camera ITS tests"
sudo apt-get install python-tk
sudo pip install numpy
sudo pip install scipy
sudo pip install matplotlib
sudo apt-get install libjpeg-dev
sudo apt-get install libtiff-dev
sudo pip install Pillow
