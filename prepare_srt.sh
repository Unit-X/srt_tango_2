#!/bin/bash

git clone https://github.com/maxsharabayko/srt.git
cd srt
git fetch --all
git checkout develop/periodic-nak-tango-2
./configure --CMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target srt_static
cd ..


