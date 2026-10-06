#include <catch2/catch_test_macros.hpp>
#include "UniqueVoidPtr.hpp"

using mg::UniqueVoidPtr;
using mg::DeleterFnPtr;

struct DeleteTracker {
    static inline int cnt = 0;
    static inline void* prev = nullptr;
    static void Reset() {
        cnt = 0;
        prev = nullptr;
    }
};

void TrackingDeleter(void* ptr) noexcept {
    DeleteTracker::cnt++;
    DeleteTracker::prev = ptr;
}

void OtherDeleter(void* ptr) noexcept {
    (void)ptr;
}

TEST_CASE("Default constructor is empty", "[UniqueVoidPtr]") {
    UniqueVoidPtr ptr;

    REQUIRE(ptr.Get() == nullptr);
    REQUIRE(ptr.GetContext() == nullptr);
    REQUIRE_FALSE(ptr);
    REQUIRE(ptr == nullptr);
}

TEST_CASE("Single argument constructor sets data, ctx is null", "UniqueVoidPtr") {
    int dummy = 0;
    UniqueVoidPtr ptr(&dummy);
    REQUIRE(ptr.Get() == &dummy);
    REQUIRE(ptr.GetContext() == nullptr);
    REQUIRE(ptr);
}

TEST_CASE("Full constructor stores data, ctx, and deleter", "[UniqueVoidPtr]") {
    DeleteTracker::Reset();
    int dummy = 0;

    {
        UniqueVoidPtr ptr(&dummy, &dummy, &TrackingDeleter);
        REQUIRE(ptr.Get() == &dummy);
        REQUIRE(ptr.GetContext() == &dummy);
        REQUIRE(ptr.GetDeleter() == &TrackingDeleter);
    }

    REQUIRE(DeleteTracker::cnt == 1);
    REQUIRE(DeleteTracker::prev == &dummy);
}

TEST_CASE("Null ctx_deleter falls back to DeleteNothing", "[UniqueVoidPtr]") {
    int dummy = 0;

    UniqueVoidPtr ptr(&dummy, &dummy, nullptr);

    REQUIRE(ptr.GetDeleter() == &mg::DeleteNothing);
}

// This is the exact edge-case with std::unique_ptr that skips the deleter if get() == nullptr
TEST_CASE(
    "Deleter fires when m_data == nullptr but m_ctx != nullptr", 
    "[UniqueVoidPtr][edge-case]") 
{
    DeleteTracker::Reset();
    int dummy = 0;

    {
        UniqueVoidPtr ptr(nullptr, &dummy, &TrackingDeleter);
        REQUIRE(ptr.Get() == nullptr);
        REQUIRE(ptr.GetContext() == &dummy);
    }

    REQUIRE(DeleteTracker::cnt == 1);
    REQUIRE(DeleteTracker::prev == &dummy);
}

TEST_CASE("Move construction transfers ownership and null source", "[UniqueVoidPtr]") {
    DeleteTracker::Reset();
    int dummy = 0;

    UniqueVoidPtr p1(&dummy, &dummy, &TrackingDeleter);
    UniqueVoidPtr p2(std::move(p1));

    REQUIRE(p2.Get() == &dummy);
    REQUIRE(p2.GetContext() == &dummy);

    // Moved-from state should be emtpy
    REQUIRE(p1.Get() == nullptr);
    REQUIRE(p1.GetContext() == nullptr);
    REQUIRE_FALSE(p1);

    // Deleter shouldn't have been called yet
    REQUIRE(DeleteTracker::cnt == 0);
}

TEST_CASE(
    "Move assignment into empty target transfers ownership and nulls move-source", 
    "[UniqueVoidPtr]") 
{
    DeleteTracker::Reset();
    int dummy = 0;

    UniqueVoidPtr p1(&dummy, &dummy, &TrackingDeleter);
    UniqueVoidPtr p2;

    p2 = std::move(p1);

    REQUIRE(p2.Get() == &dummy);
    REQUIRE(p2.GetContext() == &dummy);
    REQUIRE(p1.Get() == nullptr);
    REQUIRE(p1.GetContext() == nullptr);
    REQUIRE(DeleteTracker::cnt == 0);
}

TEST_CASE("Move assignment into a non-empty target frees old resource first", "[UniqueVoidPtr]") {
    DeleteTracker::Reset();
    int first = 0, second = 0;

    UniqueVoidPtr p1(&first, &first, &TrackingDeleter);
    UniqueVoidPtr p2(&second, &second, &TrackingDeleter);

    // Move data from p1 into p2. This should call the deleter for p2 before-so
    p2 = std::move(p1);

    REQUIRE(p2.Get() == &first);
    REQUIRE(p2.GetContext() == &first);
    REQUIRE(p1.Get() == nullptr);
    REQUIRE(p1.GetContext() == nullptr);
    REQUIRE(DeleteTracker::cnt == 1);
    REQUIRE(DeleteTracker::prev == &second);
}

TEST_CASE("Destructor only fires once, not double-freed", "[UniqueVoidPtr][regression]") {
    DeleteTracker::Reset();
    int dummy = 0;
    {
        UniqueVoidPtr ptr(&dummy, &dummy, &TrackingDeleter);

        ptr.~UniqueVoidPtr();

        REQUIRE(DeleteTracker::cnt == 1);
        new (&ptr) UniqueVoidPtr();
    }
}

TEST_CASE("Clear resets both data and ctx without invoking deleter twice", "[UniqueVoidPtr]") {
    DeleteTracker::Reset();
    int dummy = 0;
    UniqueVoidPtr ptr(&dummy, &dummy, &TrackingDeleter);

    ptr.Clear();

    REQUIRE(DeleteTracker::cnt == 1);
    REQUIRE(ptr.Get() == nullptr);
    REQUIRE(ptr.GetContext() == nullptr);
    REQUIRE_FALSE(ptr);
}

TEST_CASE("ReleaseContext transfers ownership out without invoking deleter", "[UniqueVoidPtr]") {
    DeleteTracker::Reset();
    int dummy = 0;
    UniqueVoidPtr ptr(&dummy, &dummy, &TrackingDeleter);

    {
        void* released = ptr.ReleaseContext();
        REQUIRE(released == &dummy);
        REQUIRE(ptr.GetContext() == nullptr);

        // m_data should be untouched by ReleaseContext
        REQUIRE(ptr.Get() == &dummy);

        // Deleter should not fire. The caller now owns cleanup
        REQUIRE(DeleteTracker::cnt == 0);

        // Suppose we "cleaned up" released
        TrackingDeleter(released);
    }

    REQUIRE(DeleteTracker::cnt == 1);
}

TEST_CASE("CastContext succeeds when deleter matches", "[UniqueVoidPtr]") {
    int dummy = 67;
    UniqueVoidPtr ptr(&dummy, &dummy, &TrackingDeleter);

    int* casted = ptr.CastContext<int>(&TrackingDeleter);
    REQUIRE(casted != nullptr);
    REQUIRE(*casted == 67);
}

TEST_CASE("CastContext returns nullptr when deleter mismatches", "[UniqueVoidPtr]") {
    int dummy = 67;
    UniqueVoidPtr ptr(&dummy, &dummy, &TrackingDeleter);

    int* casted = ptr.CastContext<int>(&OtherDeleter);
    REQUIRE(casted == nullptr);
}

TEST_CASE("operator-> returns data pointer", "[UniqueVoidPtr]") {
    struct Point { int x, y; };
    Point p{1, 2};
    UniqueVoidPtr ptr(&p);
    REQUIRE(ptr.operator->() == &p);
}

TEST_CASE("Equality operators against nullptr", "[UniqueVoidPtr]") {
    UniqueVoidPtr empty;
    int dummy = 0;
    UniqueVoidPtr nonempty(&dummy);

    REQUIRE(empty == nullptr);
    REQUIRE(nullptr == empty);
    REQUIRE_FALSE(empty != nullptr);

    REQUIRE(nonempty != nullptr);
    REQUIRE(nullptr != nonempty);
    REQUIRE_FALSE(nonempty == nullptr);
}

TEST_CASE("m_data and m_ctx can diverge (offset-pointer scenario)", "[UniqueVoidPtr]") {
    DeleteTracker::Reset();
    alignas(64) char block[64];
    void* offset = block + 16;
    {
        UniqueVoidPtr ptr(offset, block, &TrackingDeleter);
        REQUIRE(ptr.Get() == offset);
        REQUIRE(ptr.GetContext() == block);
        REQUIRE(ptr.Get() != ptr.GetContext());
    }
    REQUIRE(DeleteTracker::prev == block);
}