#include "leak_logic.hpp"
#include <gtest/gtest.h>

TEST(LeakLogicTest, BaseTest)
{
    auto result = hello();
    EXPECT_EQ(result, 0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}