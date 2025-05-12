#!/bin/bash

set -ex

code_dir=${PWD}
mkdir -p Build

pushd Build
    g++ \
        -Wall \
        -Werror \
        -fsanitize=address \
        -std=c++17 \
        -D DN_UNIT_TESTS_WITH_MAIN \
        -D DN_UNIT_TESTS_WITH_KECCAK \
        -x ${code_dir}/dn.cpp \
        -g \
        -o dn_unit_tests
popd
