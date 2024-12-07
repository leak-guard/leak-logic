#pragma once
#include "leakguard/leak_logic.hpp"
#include <gtest/gtest.h>

TEST(LeakLogicTest, ShouldDetectLeakWithFlowMeter) {
    lg::LeakLogic logic;
    std::array<bool, 256> probeStates {};

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
    std::array<bool, 256> probeStates {};

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
    std::array<bool, 256> probeStates {};

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

TEST(LeakLogicTests, ShouldRespondToProbeSignal) {
    lg::LeakLogic logic;
    std::array<bool, 256> probeStates {};

    // Flow rate of 3 L/min (will not be handled, TimeBasedFlowRateCriterion is missing
    // Emit signal on probe 42
    probeStates[42] = true;
    logic.update(lg::SensorState(3, probeStates), 30);

    ASSERT_EQ(logic.getAction().getActionType(), lg::ActionType::CLOSE_VALVE);
    ASSERT_EQ(logic.getAction().getActionReason(), lg::ActionReason::LEAK_DETECTED_BY_PROBE);
    ASSERT_EQ(logic.getAction().getProbeId(), 42);
}