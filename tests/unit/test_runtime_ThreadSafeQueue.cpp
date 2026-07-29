#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>

#include "runtime/ThreadSafeQueue.hh"

namespace jstine {
namespace {

TEST(ThreadSafeQueue, TransfersMoveOnlyValueOwnership) {
    ThreadSafeQueue<std::unique_ptr<int>> queue;

    EXPECT_TRUE(queue.push(std::make_unique<int>(42)));

    auto value = queue.pop();
    ASSERT_TRUE(value);
    EXPECT_EQ(**value, 42);
}

TEST(ThreadSafeQueue, CloseUnblocksWaitingConsumer) {
    ThreadSafeQueue<int> queue;
    auto consumer =
        std::async(std::launch::async, [&queue] { return queue.pop(); });

    EXPECT_EQ(
        consumer.wait_for(std::chrono::milliseconds(10)),
        std::future_status::timeout
    );

    queue.close();

    EXPECT_FALSE(consumer.get());
    EXPECT_FALSE(queue.push(42));
}

}  // namespace
}  // namespace jstine
