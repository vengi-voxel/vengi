/**
 * @file
 */

#include "app/Async.h"
#include "app/tests/AbstractTest.h"

namespace app {

class TestAsync : public app::AbstractTest {
};

TEST_F(TestAsync, testForParallel) {
    constexpr int size = 512;
    int buf[size + 1];
    buf[size] = -1;
    app::for_parallel(0, size, [&buf](int start, int end) {
        for (int i = start; i < end; ++i) {
            buf[i] = i;
        }
    });
    for (int i = 0; i < size; ++i) {
        ASSERT_EQ(buf[i], i);
    }
    // sentinel
    ASSERT_EQ(buf[size], -1);
}

} // namespace app
