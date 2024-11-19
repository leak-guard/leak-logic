#pragma once
#include "leakguard/leak_logic.hpp"
#include <gtest/gtest.h>

TEST(LeakLogicTest, ShouldDetectLeakWithFlowMeter) {
    lg::LeakLogic logic;
    lg::StaticVector<bool, 256> probeStates;

    // Detect leak if flow rate exceeds 2 L/min for at least 1 minute
    logic.addCriterion(std::make_unique<lg::TimeBasedFlowRateCriterion>(2.0f, 60));

    // Flow rate of 3 L/min for 30 secs
    logic.update(lg::SensorState(3, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::NO_ACTION);

    // Continue flow for 30 secs
    logic.update(lg::SensorState(3, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::CLOSE_VALVE);
}

TEST(LeakLogicTest, ShouldNotDetectLeakIfFlowStops) {
    lg::LeakLogic logic;
    lg::StaticVector<bool, 256> probeStates;

    // Detect leak if flow rate exceeds 2 L/min for at least 1 minute
    logic.addCriterion(std::make_unique<lg::TimeBasedFlowRateCriterion>(2.0f, 60));

    // Flow rate of 3 L/min for 30 secs
    logic.update(lg::SensorState(3, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::NO_ACTION);

    // Cut flow immediately
    logic.update(lg::SensorState(0, probeStates), 0);

    // Wait 30 seconds
    logic.update(lg::SensorState(0, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::NO_ACTION);
}

TEST(LeakLogicTest, ShouldHandleCriteriaReconfiguration) {
    lg::LeakLogic logic;
    lg::StaticVector<bool, 256> probeStates;

    // Detect leak if flow rate exceeds 2 L/min for at least 1 minute
    logic.addCriterion(std::make_unique<lg::TimeBasedFlowRateCriterion>(2.0f, 60));

    // Flow rate of 3 L/min for 30 secs
    logic.update(lg::SensorState(3, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::NO_ACTION);

    // Remove criterion
    logic.removeCriterion(0);

    // Wait 30 seconds
    logic.update(lg::SensorState(0, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::NO_ACTION);

    // Add criterion back, with a shorter period
    logic.addCriterion(std::make_unique<lg::TimeBasedFlowRateCriterion>(2.0f, 15));

    // Wait 30 seconds
    logic.update(lg::SensorState(0, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::NO_ACTION);

    // Flow rate of 3 L/min for 30 secs, should close valve
    logic.update(lg::SensorState(3, probeStates), 30);
    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::CLOSE_VALVE);
}