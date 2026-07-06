#include <gtest/gtest.h>

#include "core/Log.hh"

using namespace jstine;

TEST(LogTests, ConvertsLevelsToAndFromStrings) {
    EXPECT_EQ(log::levelMap().at("warn"), log::Level::warn);
    EXPECT_EQ(log::levelFromString("debug"), log::Level::debug);
    EXPECT_EQ(log::levelFromString("not-a-level"), log::Level::info);
    EXPECT_EQ(log::levelToString(log::Level::critical), "critical");
}

TEST(LogTests, SetLogLevelUpdatesGlobalState) {
    const auto previous = log::level();

    log::setLogLevel(log::Level::trace);
    EXPECT_EQ(log::level(), log::Level::trace);

    log::setLogLevel(log::Level::off);
    EXPECT_EQ(log::level(), log::Level::off);

    log::setLogLevel(previous);
}
