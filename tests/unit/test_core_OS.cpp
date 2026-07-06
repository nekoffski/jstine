#include <gtest/gtest.h>

#include "core/OS.hh"

using namespace jstine;

TEST(OSTests, FormatsKnownPlatforms) {
    EXPECT_EQ(toString(OS::linux), "Linux");
    EXPECT_EQ(toString(OS::windows), "Windows");
    EXPECT_EQ(toString(OS::darwin), "Darwin");
}
