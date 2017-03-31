#!/usr/bin/env bash

if [ $# -eq 0 ]
then
  echo "If you intend to use this shell script, please look it over and make sure you use it properly."
  echo "To continue, pass your compiler as an argument to this script (eg, ./install-dependencies.sh g++-6 )"
  echo "Clang will default to 3.9 and gcc will default to 6. Edit me if you want to change this behavior."
  exit 1
fi

set -ex

BASE_DIR="$(pwd)/lib"

if [ ! -d lib ]; then
	mkdir lib
fi

COMPILER_PARAM="${1}"

if [[ ${1} =~ "gcc" ]]
then
  export CXX=g++
  export CC=gcc
  export COMPILER=g++-6
elif [[ ${1} =~ "g++" ]]
then
  export CXX=g++
  export CC=gcc
  export COMPILER=g++-6
elif [[ ${1} =~ "clang" ]]
then
  export LLVM_VERSION=3.9.0
  export CC=clang
  export CXX=clang
  export COMPILER=clang3.9
fi

export BOOST_VERSION=default
export POCO_VERSION=1.7.8

gcc --version

DEPS_DIR="$(pwd)/lib"
cd ${DEPS_DIR}

if [[ "${LLVM_VERSION}" == "default" ]]; then LLVM_VERSION=3.9.0; fi
if [[ "${BOOST_VERSION}" == "default" ]]; then BOOST_VERSION=1.63.0; fi

if [[ "${COMPILER}" != "" ]]; then export CXX=${COMPILER}; fi

if [[ "${BOOST_VERSION}" != "" ]]; then
  BOOST_DIR=${DEPS_DIR}/boost-${BOOST_VERSION}
  if [[ -z "$(ls -A ${BOOST_DIR})" ]]; then
    BOOST_URL="http://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/boost_${BOOST_VERSION//\./_}.tar.gz"
    mkdir -p ${BOOST_DIR}
    { wget --quiet -O - ${BOOST_URL} | tar --strip-components=1 -xz -C ${BOOST_DIR}; } || exit 1
  fi
  CMAKE_OPTIONS+=" -DBOOST_ROOT=${BOOST_DIR}"
fi
if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
  CMAKE_URL="https://cmake.org/files/v3.7/cmake-3.7.2-Linux-x86_64.tar.gz"
  mkdir cmake && wget --no-check-certificate --quiet -O - ${CMAKE_URL} | tar --strip-components=1 -xz -C cmake
  export PATH=${DEPS_DIR}/cmake/bin:${PATH}
fi
cmake --version

if [[ "${LLVM_VERSION}" != "" ]]; then
    LLVM_DIR=${DEPS_DIR}/llvm-${LLVM_VERSION}
    if [[ -z "$(ls -A ${LLVM_DIR})" ]]; then
    LLVM_URL="http://llvm.org/releases/${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz"
    LIBCXX_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxx-${LLVM_VERSION}.src.tar.xz"
    LIBCXXABI_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxxabi-${LLVM_VERSION}.src.tar.xz"
    CLANG_URL="http://llvm.org/releases/${LLVM_VERSION}/clang+llvm-${LLVM_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
    mkdir -p ${LLVM_DIR} ${LLVM_DIR}/build ${LLVM_DIR}/projects/libcxx ${LLVM_DIR}/projects/libcxxabi ${LLVM_DIR}/clang
    wget --quiet -O - ${LLVM_URL}      | tar --strip-components=1 -xJ -C ${LLVM_DIR}
    wget --quiet -O - ${LIBCXX_URL}    | tar --strip-components=1 -xJ -C ${LLVM_DIR}/projects/libcxx
    wget --quiet -O - ${LIBCXXABI_URL} | tar --strip-components=1 -xJ -C ${LLVM_DIR}/projects/libcxxabi
    wget --quiet -O - ${CLANG_URL}     | tar --strip-components=1 -xJ -C ${LLVM_DIR}/clang
    (cd ${LLVM_DIR}/build && cmake .. -DCMAKE_INSTALL_PREFIX=${LLVM_DIR}/install -DCMAKE_CXX_COMPILER=clang++)
    (cd ${LLVM_DIR}/build/projects/libcxx && make install -j2)
    (cd ${LLVM_DIR}/build/projects/libcxxabi && make install -j2)
    fi
    export CXXFLAGS="-nostdinc++ -isystem ${LLVM_DIR}/install/include/c++/v1"
    export LDFLAGS="-L ${LLVM_DIR}/install/lib -l c++ -l c++abi"
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${LLVM_DIR}/install/lib"
    export PATH="${LLVM_DIR}/clang/bin:${PATH}"
fi
${CXX} --version

(cd ${BOOST_DIR} && ./bootstrap.sh && ./b2 toolset=${CC} --with-thread --with-system --with-program_options --with-date_time --with-test --with-log --with-iostreams)

POCO_DIR=${DEPS_DIR}/poco-${POCO_VERSION}
POCO_URL="https://pocoproject.org/releases/poco-${POCO_VERSION}/poco-${POCO_VERSION}-all.tar.gz"
mkdir -p ${POCO_DIR}
wget --quiet -O - ${POCO_URL}      | tar --strip-components=1 -xz -C ${POCO_DIR}
if [[ "${LLVM_VERSION}" != "" ]]; then
    (cd ${POCO_DIR} && cmake . -DCMAKE_CXX_COMPILER=${CXX} && make)
else
    (cd ${POCO_DIR} && cmake . -DCMAKE_CXX_COMPILER=${CXX} && make)
fi

if [ ! -f "${DEPS_DIR}/fmt/fmt/libfmt.a" ]; then
    (cd ${DEPS_DIR}/fmt && cmake . && make)
fi

cd ${BASE_DIR}
(cmake . ${CMAKE_OPTIONS})
make