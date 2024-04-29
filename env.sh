#!/bin/sh

pushd $(dirname "${BASH_SOURCE[0]}")
pwd=$(pwd)
addpath PATH $pwd
addpath CPATH $pwd
addpath LIBRARY_PATH $pwd
popd