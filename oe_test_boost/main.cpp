#include <iostream>

#define BOOST_TEST_MODULE TestName

#include <boost/test/included/unit_test.hpp>
#include "fixture_local.h"

BOOST_FIXTURE_TEST_SUITE( oe_test_suite, oe_fixture)

BOOST_AUTO_TEST_CASE( oe_test_one){
    BOOST_CHECK(true);
    BOOST_CHECK(true);
}
BOOST_AUTO_TEST_SUITE_END()


//
//  The fixtures initiates and destructs
//  the definable MyFixture for every test
//  case below (BOOST_AUTO_TEST_CASE)
//
BOOST_FIXTURE_TEST_SUITE( example_suite, oe_fixture)

BOOST_AUTO_TEST_CASE(test_one){
  BOOST_CHECK(true);
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(test_two){
  BOOST_CHECK(true);
  BOOST_CHECK(true);
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
