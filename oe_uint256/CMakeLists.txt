cmake_minimum_required(VERSION 3.15)
project(oe_uint256)

set(CMAKE_CXX_STANDARD 11)

add_executable(oe_uint256 main.cpp uint256.cpp  arith_uint256.cpp util/strencodings.cpp)