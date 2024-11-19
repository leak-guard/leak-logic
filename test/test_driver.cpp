#include "suites/leak_logic_tests.hpp"

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

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}