#pragma once
#include "leakguard/leak_logic.hpp"
#include <gtest/gtest.h>

TEST(SerializationTests, ShouldSerializeTimeBasedFlowCriterion) {
    const lg::TimeBasedFlowRateCriterion criterion(1.673, 1234);
    const auto serialized = criterion.serialize();
    ASSERT_STREQ(serialized.ToCStr(), "T,167,1234,");
}

TEST(SerializationTests, ShouldDeserializeTimeBasedFlowCriterion) {
    const lg::StaticString<16> serialized("T,167,1234,");
    const auto deserialized = lg::TimeBasedFlowRateCriterion::deserialize(serialized);
    ASSERT_NEAR(deserialized->getRateThreshold(), 1.67, 0.01);
    ASSERT_EQ(deserialized->getMinDuration(), 1234);
}

TEST(SerializationTests, ShouldSerializeMultipleCriteria) {
    lg::LeakLogic logic;
    logic.addCriterion(std::make_unique<lg::TimeBasedFlowRateCriterion>(2.0f, 60));
    logic.addCriterion(std::make_unique<lg::TimeBasedFlowRateCriterion>(5.0f, 120));
    const auto serialized = logic.serialize();

    ASSERT_STREQ(serialized.ToCStr(), "T,200,60,|T,500,120,|");
}

TEST(SerializationTests, ShouldDeserializeMultipleCriteria) {
    lg::LeakLogic logic;
    logic.loadFromString("T,200,60,|T,500,120,|");
    const auto serialized = logic.serialize();
    ASSERT_STREQ(serialized.ToCStr(), "T,200,60,|T,500,120,|");
}