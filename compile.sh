#!/bin/sh

set -xe

g++ main.cpp -o hot-swap -std=c++20 -lfmt
g++ -fPIC -shared -o libFoo1.so libFoo1.cpp
g++ -fPIC -shared -o libFoo2.so libFoo2.cpp
