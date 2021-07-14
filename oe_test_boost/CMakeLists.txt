cmake_minimum_required(VERSION 3.15)
project(oe_test_boost)
enable_testing()
#find_package(Boost XX REQUIRED COMPONENTS unit_test_framework)

set(CMAKE_CXX_STANDARD 11)
find_package(Boost COMPONENTS program_options)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

add_executable(oe_test_boost
        ab.test
        main.cpp
        oe_uint256_tests.cpp
        uint256.cpp  arith_uint256.cpp util/strencodings.cpp
        )

message(STATUS ${Boost_LIBRARIES})
