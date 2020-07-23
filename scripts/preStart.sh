#!/bin/sh

echo "Running fpp-zcpp PreStart Script"

BASEDIR=$(dirname $0)
cd $BASEDIR
cd ..
make
