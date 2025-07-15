#include <gtest/gtest.h>
#include <iostream>

TEST(SkipTest, TestSkip) {
    GTEST_SKIP() << "This test should be skipped";
    std::cout << "This should not be printed" << std::endl;
    EXPECT_TRUE(false) << "This should not fail";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
EOF < /dev/null