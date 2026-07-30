#include <gtest/gtest.h>

#include <cstdlib>

#include "storage/kv/EmptyValue.hh"
#include "storage/kv/StrValue.hh"
#include "storage/kv/ValueWrapper.hh"

using namespace jstine;

namespace {

class TrackingAllocator : public Allocator {
   public:
    void* allocate(u64 size) override {
        ++allocations;
        return std::malloc(size);
    }

    void free(void* ptr) override {
        ++frees;
        std::free(ptr);
    }

    u64 allocations = 0;
    u64 frees = 0;
};

class FailingAllocator : public Allocator {
   public:
    void* allocate(u64) override { return nullptr; }
    void free(void*) override {}
};

}  // namespace

TEST(ValueTests, EmptyValueHasNoBytes) {
    EXPECT_TRUE(EmptyValue{}.bytes().empty());
}

TEST(ValueTests, StrValueCopiesInputAndReleasesAllocation) {
    TrackingAllocator allocator;
    Bytes input{'v', 'a', 'l'};

    {
        auto parsed = StrValue::parse(&allocator, input);
        ASSERT_TRUE(parsed);
        input[0] = 'x';
        EXPECT_EQ(
            Bytes(parsed->bytes().begin(), parsed->bytes().end()),
            (Bytes{'v', 'a', 'l'})
        );
        EXPECT_EQ(allocator.allocations, 1u);
        EXPECT_EQ(allocator.frees, 0u);
    }

    EXPECT_EQ(allocator.frees, 1u);
}

TEST(ValueTests, StrValueReportsAllocationFailure) {
    FailingAllocator allocator;

    auto parsed = StrValue::parse(&allocator, Bytes{'v'});

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code(), ErrorCode::allocatorFailure);
}

TEST(ValueTests, StrValueMoveAssignmentReleasesPreviousValue) {
    TrackingAllocator allocator;
    auto first = StrValue::parse(&allocator, Bytes{'a'});
    auto second = StrValue::parse(&allocator, Bytes{'b'});
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    {
        StrValue value = std::move(*first);
        value = std::move(*second);
        EXPECT_EQ(
            Bytes(value.bytes().begin(), value.bytes().end()), (Bytes{'b'})
        );
        EXPECT_TRUE(first->bytes().empty());
        EXPECT_TRUE(second->bytes().empty());
        EXPECT_EQ(allocator.frees, 1u);

        value = std::move(value);
        EXPECT_EQ(
            Bytes(value.bytes().begin(), value.bytes().end()), (Bytes{'b'})
        );
        EXPECT_EQ(allocator.frees, 1u);
    }

    EXPECT_EQ(allocator.frees, 2u);
}

TEST(ValueTests, ValueWrapperStartsEmptyAndExposesMetadata) {
    ValueWrapper wrapper;

    EXPECT_TRUE(wrapper.bytes().empty());
    EXPECT_EQ(&wrapper.metadata(), &wrapper.metadata());
}

TEST(ValueTests, ValueWrapperStoresStringValue) {
    TrackingAllocator allocator;
    ValueWrapper wrapper;

    EXPECT_FALSE(wrapper.set(Bytes{'v'}, allocator));
    EXPECT_EQ(
        Bytes(wrapper.bytes().begin(), wrapper.bytes().end()), (Bytes{'v'})
    );
}

TEST(ValueTests, ValueWrapperPreservesEmptyValueOnAllocationFailure) {
    FailingAllocator allocator;
    ValueWrapper wrapper;

    const auto error = wrapper.set(Bytes{'v'}, allocator);

    ASSERT_TRUE(error);
    EXPECT_EQ(error->code(), ErrorCode::allocatorFailure);
    EXPECT_TRUE(wrapper.bytes().empty());
}
