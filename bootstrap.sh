#!/bin/bash

if [ "$EUID" -ne 0 ]; then 
  echo "This script should be run as root"
  exit
fi

DEPS="libbison-dev libfl-dev flex bison"
echo "Installing dependencies: $DEPS"
apt install $DEPS
