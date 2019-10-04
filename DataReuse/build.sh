#!/bin/bash

rm -r .build/
mkdir .build
cd .build/
cmake -DLLVM_ENABLE_RTTI=true ..
make
cd ../
