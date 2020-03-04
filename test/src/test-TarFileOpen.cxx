#include "ootoc.h"
#include <gtest/gtest.h>

using namespace ootoc;
TEST(TarOverCurlSet, fileopen)
{
    ASSERT_EXIT(TarOverCurlSet("test/data/Unknow.tar"), ::testing::ExitedWithCode(1), "");
    TarOverCurlSet("test/data/Packages.tar");
}
