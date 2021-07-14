cmake_minimum_required(VERSION 3.15)
project(btc_du)

set(CMAKE_CXX_STANDARD 11)

add_executable(btc_du main.cpp uint256.cpp  arith_uint256.cpp util/strencodings.cpp)
