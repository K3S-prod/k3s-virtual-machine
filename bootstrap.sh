#!/bin/bash

if [ "$EUID" -ne 0 ]; then 
  echo "This script should be run as root"
  exit
fi

DEPS="libbison-dev libfl-dev libruby-dev libruby flex bison ruby-full ninja-build"
echo "Installing dependencies: $DEPS"

apt update
apt install $DEPS
