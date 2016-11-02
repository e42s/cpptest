#include "cpptest.hpp"
#include <stdexcept>


TESTCASE("first testcase") {
  SECTION("a") {
    throw ::std::runtime_error("error");
  }

  SECTION("b") {
    throw 1;
  }

  SECTION("c") {
    SECTION("d") {
      ASSERT(1==1);
    }

    ASSERT(1==2);
  }

  SECTION("e") {
    ASSERT_THROW(int, throw 1);
    ASSERT_THROW(int, );
  }
}
