#!/bin/bash
set -e

qmake6 xcom.pro
make -j$(nproc)
cp build/xcom ./xcom
echo "Done: ./xcom"
