#include <catch2/catch_test_macros.hpp>

#define MINIGRAD_TESTS
#include "IntrusivePtr.hpp"

using mg::MakeIntrusive;
using mg::IntrusivePtrTarget;
using mg::IntrusivePtr;

class TestObject : public IntrusivePtrTarget {
public:
    explicit TestObject(int val = 0)
        : m_val(val)
    { 
        live_cnt++; 
    }

    ~TestObject() override {
        --live_cnt;
        ++destructor_cnt;
    }

    int m_val;

    static inline int live_cnt = 0;
    static inline int destructor_cnt = 0;
};

class DerivedTestObject : public TestObject {
public:
    explicit DerivedTestObject(int value = 0) : TestObject(value) {}
};

class InspectableTarget : public IntrusivePtrTarget {
public:
    uint32_t Refcount() const { return IntrusivePtrTarget::TestingRefcount(); }
    uint32_t Weakcount() const { return IntrusivePtrTarget::TestingWeakcount(); }
};

void ResetCounters() {
    TestObject::live_cnt = 0;
    TestObject::destructor_cnt = 0;
}

TEST_CASE("IntrusivePtrTarget starts with zero reference count", "[IntrusivePtrTarget]") {
    InspectableTarget target;

    REQUIRE(target.Refcount() == 0);
    REQUIRE(target.Weakcount() == 0);
}

TEST_CASE(
    "IntrusivePtrTarget copy construction does not copy reference counts", 
    "[IntrusivePtrTarget]") 
{
    InspectableTarget source;
    InspectableTarget copy(source);

    REQUIRE(source.Refcount() == 0);
    REQUIRE(source.Weakcount() == 0);

    REQUIRE(copy.Refcount() == 0);
    REQUIRE(copy.Weakcount() == 0);
}

TEST_CASE(
    "IntrusivePtrTarget copy assignment does not copy reference counts", 
    "[IntrusivePtrTarget]")
{
    InspectableTarget source;
    InspectableTarget destination;

    destination = source;

    REQUIRE(source.Refcount() == 0);
    REQUIRE(source.Weakcount() == 0);

    REQUIRE(destination.Refcount() == 0);
    REQUIRE(destination.Weakcount() == 0);
}

TEST_CASE(
    "IntrusivePtrTarget move constructor starts with zero reference counts", 
    "[IntrusivePtrTarget]") 
{
    InspectableTarget source;
    InspectableTarget moved(std::move(source));

    REQUIRE(source.Refcount() == 0);
    REQUIRE(source.Weakcount() == 0);

    REQUIRE(moved.Refcount() == 0);
    REQUIRE(moved.Weakcount() == 0);
}

TEST_CASE(
    "IntrusivePtrTarget move assignment starts with zero reference counts", 
    "[IntrusivePtrTarget]") 
{
    InspectableTarget source;
    InspectableTarget moved = std::move(source);

    REQUIRE(source.Refcount() == 0);
    REQUIRE(source.Weakcount() == 0);

    REQUIRE(moved.Refcount() == 0);
    REQUIRE(moved.Weakcount() == 0);
}

TEST_CASE("Default constructed IntrusivePtr is empty", "[IntrusivePtr]") {
    ResetCounters();
    IntrusivePtr<TestObject> ptr;

    REQUIRE(ptr.Get() == nullptr);
    REQUIRE(ptr == nullptr);
    REQUIRE_FALSE(ptr);
    REQUIRE_FALSE(ptr.Defined());
    REQUIRE(ptr.UseCount() == 0);
    REQUIRE(ptr.WeakUseCount() == 0);
    REQUIRE_FALSE(ptr.Unique());
}

TEST_CASE(
    "MakeIntrusive creates an object with one strong ref and weak ref",
    "[IntrusivePtr]")
{
    ResetCounters();
    {
        auto ptr = MakeIntrusive<TestObject>(67);

        REQUIRE(ptr.Get() != nullptr);
        REQUIRE(ptr->m_val == 67);

        REQUIRE(ptr.UseCount() == 1);
        REQUIRE(ptr.WeakUseCount() == 1);
        REQUIRE(ptr.Unique());
        REQUIRE(ptr.Defined());

        REQUIRE(TestObject::live_cnt == 1);
    }

    REQUIRE(TestObject::live_cnt == 0);
    REQUIRE(TestObject::destructor_cnt == 1);
}

TEST_CASE(
    "IntrusivePtr copy construction increments the strong count",
    "[IntrusivePtr]")
{
    ResetCounters();
    {
        auto ptr = MakeIntrusive<TestObject>(67);
        REQUIRE(ptr.UseCount() == 1);
        {
            auto copy = ptr;

            REQUIRE(copy.Get() == ptr.Get());
            REQUIRE(ptr.UseCount() == 2);
            REQUIRE(copy.UseCount() == 2);
            REQUIRE(ptr.WeakUseCount() == 1);
            REQUIRE(copy.WeakUseCount() == 1);

            REQUIRE_FALSE(ptr.Unique());
            REQUIRE_FALSE(copy.Unique());
        }
        REQUIRE(ptr.UseCount() == 1);
        REQUIRE(ptr.WeakUseCount() == 1);
        REQUIRE(ptr.Unique());
    }
}

TEST_CASE("IntrusivePtr copy of a nullptr remains nullptr", "[IntrusivePtr]") {
    IntrusivePtr<TestObject> ptr;

    auto copy = ptr;
    REQUIRE(copy.Get() == nullptr);
    REQUIRE(copy.UseCount() == 0);
    REQUIRE(copy.WeakUseCount() == 0);
}

TEST_CASE("IntrusivePtr copy assignment replaces the old object", "[IntrusivePtr]") {
    ResetCounters();

    {
        auto first = MakeIntrusive<TestObject>(67);
        auto second = MakeIntrusive<TestObject>(69);

        REQUIRE(first.UseCount() == 1);
        REQUIRE(second.UseCount() == 1);
        REQUIRE(TestObject::live_cnt == 2);

        first = second;

        REQUIRE(first.Get() == second.Get());
        REQUIRE(first->m_val == 69);

        REQUIRE(first.UseCount() == 2);
        REQUIRE(second.UseCount() == 2);

        REQUIRE(TestObject::destructor_cnt == 1);
    }

    REQUIRE(TestObject::live_cnt == 0);
    REQUIRE(TestObject::destructor_cnt == 2);
}

TEST_CASE("IntrusivePtr move construction transfers ownership", "[IntrusivePtr]") {
    ResetCounters();

    {
        auto ptr = MakeIntrusive<TestObject>(67);
        TestObject* original = ptr.Get();

        auto moved = std::move(ptr);

        REQUIRE(moved.Get() == original);
        REQUIRE(moved->m_val == 67);
        REQUIRE(moved.UseCount() == 1);
        REQUIRE(moved.WeakUseCount() == 1);
        REQUIRE(moved.Unique());

        REQUIRE(ptr.Get() == nullptr);
        REQUIRE(ptr.UseCount() == 0);
        REQUIRE(ptr.WeakUseCount() == 0);
    }

    REQUIRE(TestObject::destructor_cnt == 1);
}

TEST_CASE("IntrusivePtr move assignment transfers ownership", "[IntrusivePtr]") 
{
    ResetCounters();

    {
        auto first = MakeIntrusive<TestObject>(67);
        auto second = MakeIntrusive<TestObject>(69);

        TestObject* second_object = second.Get();

        first = std::move(second);

        REQUIRE(first.Get() == second_object);
        REQUIRE(first->m_val == 69);

        REQUIRE(second.Get() == nullptr);
        REQUIRE(second.UseCount() == 0);

        REQUIRE(first.UseCount() == 1);
        REQUIRE(first.Unique());

        REQUIRE(TestObject::destructor_cnt == 1);
    }

    REQUIRE(TestObject::destructor_cnt == 2);
}

TEST_CASE("IntrusivePtr Reset releases the owned object", "[IntrusivePtr]") 
{
    ResetCounters();

    {
        auto ptr = MakeIntrusive<TestObject>(67);

        REQUIRE(ptr.UseCount() == 1);
        REQUIRE(TestObject::live_cnt == 1);

        ptr.Reset();

        REQUIRE(ptr.Get() == nullptr);
        REQUIRE(ptr.UseCount() == 0);
        REQUIRE(ptr.WeakUseCount() == 0);
        REQUIRE_FALSE(ptr);
        REQUIRE_FALSE(ptr.Defined());

        REQUIRE(TestObject::live_cnt == 0);
        REQUIRE(TestObject::destructor_cnt == 1);
    }
}


TEST_CASE("IntrusivePtr dereference operators access the target", "[IntrusivePtr]") 
{
    auto ptr = MakeIntrusive<TestObject>(42);

    REQUIRE((*ptr).m_val == 42);
    REQUIRE(ptr->m_val == 42);
}

TEST_CASE("IntrusivePtr equality compares target pointers", "[IntrusivePtr]") 
{
    auto first = MakeIntrusive<TestObject>(42);
    auto second = first;
    auto different = MakeIntrusive<TestObject>(42);

    REQUIRE(first == second);
    REQUIRE(first == first);

    REQUIRE_FALSE(first == different);

    IntrusivePtr<TestObject> null;

    REQUIRE(null == nullptr);
    REQUIRE_FALSE(first == nullptr);
}

TEST_CASE("IntrusivePtr Unique reflects strong ownership", "[IntrusivePtr]") 
{
    auto ptr = MakeIntrusive<TestObject>();

    REQUIRE(ptr.Unique());

    {
        auto copy = ptr;

        REQUIRE_FALSE(ptr.Unique());
        REQUIRE_FALSE(copy.Unique());
    }

    REQUIRE(ptr.Unique());
}

TEST_CASE("IntrusivePtr hash is based on the target pointer", "[IntrusivePtr]") {
    auto ptr = MakeIntrusive<TestObject>();

    std::hash<mg::IntrusivePtr<TestObject>> hasher;

    REQUIRE(hasher(ptr) == std::hash<TestObject*>{}(ptr.Get()));
}

TEST_CASE("IntrusivePtr supports converting copy construction", "[IntrusivePtr]") 
{
    ResetCounters();

    {
        auto derived = MakeIntrusive<DerivedTestObject>(42);

        IntrusivePtr<TestObject> base = derived;

        REQUIRE(base.Get() == derived.Get());
        REQUIRE(base->m_val == 42);

        REQUIRE(derived.UseCount() == 2);
        REQUIRE(base.UseCount() == 2);
    }

    REQUIRE(TestObject::destructor_cnt == 1);
}

TEST_CASE("IntrusivePtr supports converting move construction", "[IntrusivePtr]") {
    ResetCounters();

    {
        auto derived = MakeIntrusive<DerivedTestObject>(42);

        TestObject* original = derived.Get();

        IntrusivePtr<TestObject> base = std::move(derived);

        REQUIRE(base.Get() == original);
        REQUIRE(base->m_val == 42);

        REQUIRE(derived.Get() == nullptr);
        REQUIRE(derived.UseCount() == 0);

        REQUIRE(base.UseCount() == 1);
        REQUIRE(base.Unique());
    }

    REQUIRE(TestObject::destructor_cnt == 1);
}

TEST_CASE("IntrusivePtr supports converting copy assignment", "[IntrusivePtr]") {
    ResetCounters();

    {
        auto derived = MakeIntrusive<DerivedTestObject>(42);
        auto base = MakeIntrusive<TestObject>(10);

        base = derived;

        REQUIRE(base.Get() == derived.Get());
        REQUIRE(base->m_val == 42);

        REQUIRE(base.UseCount() == 2);
        REQUIRE(derived.UseCount() == 2);

        REQUIRE(TestObject::destructor_cnt == 1);
    }

    REQUIRE(TestObject::destructor_cnt == 2);
}

TEST_CASE("IntrusivePtr supports converting move assignment", "[IntrusivePtr]") {
    ResetCounters();

    {
        auto derived = MakeIntrusive<DerivedTestObject>(42);
        auto base = MakeIntrusive<TestObject>(10);

        TestObject* original = derived.Get();

        base = std::move(derived);

        REQUIRE(base.Get() == original);
        REQUIRE(base->m_val == 42);

        REQUIRE(derived.Get() == nullptr);
        REQUIRE(derived.UseCount() == 0);

        REQUIRE(base.UseCount() == 1);
    }

    REQUIRE(TestObject::destructor_cnt == 2);
}