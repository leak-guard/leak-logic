#include "suites/leak_logic_tests.hpp"
#include "suites/serialization_tests.hpp"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}