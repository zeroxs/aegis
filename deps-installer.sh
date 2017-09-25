#!/usr/bin/env bash

git submodule update --init --recursive

sudo apt -yq install software-properties-common
sudo apt-add-repository -y "ppa:ubuntu-toolchain-r/test"
sudo apt update
sudo apt -yq --no-install-suggests --no-install-recommends install g++-6 git gcc g++ make libssl-dev

export COMPILER=g++-6
export COMPILERGCC=gcc-6
export BOOST_VERSION=1.63.0
export POCO_VERSION=1.7.8
export CXX=g++
export CC=gcc



DEPS_DIR="${PWD}/lib"
mkdir -p ${DEPS_DIR} && cd ${DEPS_DIR}



CMAKE_URL="https://cmake.org/files/v3.7/cmake-3.7.2-Linux-x86_64.tar.gz"
mkdir cmake && wget --no-check-certificate --quiet -O - ${CMAKE_URL} | tar --strip-components=1 -xz -C cmake
export PATH=${DEPS_DIR}/cmake/bin:${PATH}

cmake --version



BOOST_DIR=${DEPS_DIR}/boost-${BOOST_VERSION}
BOOST_URL="http://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/boost_${BOOST_VERSION//\./_}.tar.gz"
mkdir -p ${BOOST_DIR}
wget --quiet -O - ${BOOST_URL} | tar --strip-components=1 -xz -C ${BOOST_DIR};
CMAKE_OPTIONS+=" -DBOOST_ROOT=${BOOST_DIR}"

(cd ${BOOST_DIR} && ./bootstrap.sh && ./b2 toolset=${COMPILERGCC} --with-thread --with-system --with-program_options --with-date_time --with-test --with-log --with-iostreams)



POCO_DIR=${DEPS_DIR}/poco-${POCO_VERSION}
POCO_URL="https://pocoproject.org/releases/poco-${POCO_VERSION}/poco-${POCO_VERSION}-all.tar.gz"
mkdir -p ${POCO_DIR}
wget --quiet -O - ${POCO_URL} | tar --strip-components=1 -xz -C ${POCO_DIR}
(cd ${POCO_DIR} && cmake . -DCMAKE_CXX_COMPILER=${COMPILER} -DCMAKE_C_COMPILER=${COMPILERGCC} && make)



(cd ${DEPS_DIR}/fmt && cmake . && make)


cd ${DEPS_DIR}/..

(cmake . -DCMAKE_CXX_COMPILER=${COMPILER} -DCMAKE_C_COMPILER=${COMPILERGCC} ${CMAKE_OPTIONS})
