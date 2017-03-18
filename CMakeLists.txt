cmake_minimum_required(VERSION 2.8)
project(aegisbot)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(aegisbot ${sources})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14 -Wall")

set(Boost_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/boost-1.63.0)
set(Boost_LIBRARY_DIR ${Boost_INCLUDE_DIR}/stage/lib)
set(Poco_DIR ${CMAKE_SOURCE_DIR}/lib/poco-1.7.8)
set(Poco_LIBRARY_DIR ${Poco_DIR}/lib)

ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK -DUSE_REDIS -DROOTADMIN=171000788183678976LL)

target_include_directories(aegisbot PUBLIC src)

include_directories(${Boost_INCLUDE_DIR} ${Poco_DIR}/Foundation/include ${Poco_DIR}/Net/include ${Poco_DIR}/NetSSL/include ${Poco_DIR}/NetSSL_OpenSSL/include ${Poco_DIR}/Crypto/include ${CMAKE_SOURCE_DIR}/lib/json/src ${CMAKE_SOURCE_DIR}/lib/redisclient/src ${CMAKE_SOURCE_DIR}/lib/websocketpp ${CMAKE_SOURCE_DIR}/lib/fmt)

link_directories(${Boost_LIBRARY_DIR} ${Poco_LIBRARY_DIR})

MESSAGE( STATUS "MAKE_SOURCE_DIR     " ${CMAKE_SOURCE_DIR} )
MESSAGE( STATUS "Boost_INCLUDE_DIR:  " ${Boost_INCLUDE_DIR} )
MESSAGE( STATUS "Boost_LIBRARY_DIR:  " ${Boost_LIBRARY_DIR} )
MESSAGE( STATUS "Poco_LIBRARY_DIR:   " ${Poco_LIBRARY_DIR} )

target_link_libraries(aegisbot pthread ${Poco_LIBRARY_DIR}/libPocoNet.so ${Poco_LIBRARY_DIR}/libPocoNetSSL.so ${Poco_LIBRARY_DIR}/libPocoFoundation.so ssl crypto ${Boost_LIBRARY_DIR}/libboost_program_options.so ${Boost_LIBRARY_DIR}/libboost_thread.so ${Boost_LIBRARY_DIR}/libboost_system.so ${CMAKE_SOURCE_DIR}/lib/fmt/fmt/libfmt.a)
