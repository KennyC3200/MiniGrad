#include "Storage.hpp"
#include "StorageImpl.hpp"
#include "CPUAllocator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace mg;

TEST_CASE("Storage allocating ctor reports correct size", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 128, GetCPUAllocator());
    REQUIRE(s.SizeBytes() == 128);
}

TEST_CASE("Storage allocating ctor produces a non-null, writable buffer", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 64, GetCPUAllocator());
    REQUIRE(s.MutableData() != nullptr);

    // Actually write through the pointer and read it back — verifies the
    // DataPtr from the allocator is real, owned memory, not just a stub.
    std::memset(s.MutableData(), 0xAB, 64);
    auto* bytes = static_cast<unsigned char*>(s.MutableData());
    for (std::size_t i = 0; i < 64; ++i) {
        REQUIRE(bytes[i] == 0xAB);
    }
    REQUIRE(std::memcmp(s.Data(), s.MutableData(), 64) == 0);
}

TEST_CASE("Storage allocating ctor: zero-size allocation is well-defined", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 0, GetCPUAllocator());
    REQUIRE(s.SizeBytes() == 0);
    // Not asserting Data() is non-null here: a zero-byte malloc is allowed
    // to return either nullptr or a unique non-dereferenceable pointer.
}

TEST_CASE("Storage allocating ctor defaults to non-resizable", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 32, GetCPUAllocator());
    REQUIRE_FALSE(s.Resizable());
}

TEST_CASE("Storage allocating ctor threads the resizable flag through", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 32, GetCPUAllocator(), /*resizable=*/true);
    REQUIRE(s.Resizable());
}

TEST_CASE("Storage allocating ctor records the allocator it was given", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 16, GetCPUAllocator());
    REQUIRE(s.Allocator() == GetCPUAllocator());
}

TEST_CASE("Storage allocating ctor tags the storage with the allocator's device", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 16, GetCPUAllocator());
    REQUIRE(s.DeviceType() == DeviceType::CPU);
    REQUIRE(s.Device().IsCPU());
}

// Pre-allocated DataPtr ctor
TEST_CASE("Storage DataPtr ctor takes ownership of already-allocated memory", "[Storage]") {
    DataPtr existing = GetCPUAllocator()->Allocate(48);
    void* raw = existing.Get();
    REQUIRE(raw != nullptr);

    Storage s(Storage::use_byte_size_t{}, 48, std::move(existing), GetCPUAllocator());

    // The Storage now owns exactly the memory we allocated — same address,
    // not a fresh allocation.
    REQUIRE(s.Data() == raw);
    REQUIRE(s.SizeBytes() == 48);
}

TEST_CASE("Storage DataPtr ctor: data written before construction survives", "[Storage]") {
    DataPtr existing = GetCPUAllocator()->Allocate(8);
    std::memcpy(existing.Get(), "abcdefg", 8);

    Storage s(Storage::use_byte_size_t{}, 8, std::move(existing), GetCPUAllocator());

    REQUIRE(std::memcmp(s.Data(), "abcdefg", 8) == 0);
}

TEST_CASE("Storage DataPtr ctor threads the resizable flag through", "[Storage]") {
    DataPtr existing = GetCPUAllocator()->Allocate(8);
    Storage s(Storage::use_byte_size_t{}, 8, std::move(existing), GetCPUAllocator(), /*resizable=*/true);
    REQUIRE(s.Resizable());
}

TEST_CASE("Default-constructed Storage is falsy and holds no StorageImpl", "[Storage]") {
    Storage s;
    REQUIRE_FALSE(static_cast<bool>(s));
}

TEST_CASE("A constructed Storage is truthy", "[Storage]") {
    Storage s(Storage::use_byte_size_t{}, 8, GetCPUAllocator());
    REQUIRE(static_cast<bool>(s));
}

// Refcounting / sharing semantics — the whole point of routing through
// IntrusivePtr<StorageImpl> rather than owning StorageImpl directly.
TEST_CASE("Freshly constructed Storage is unique with use count 1", "[Storage][refcount]") {
    Storage s(Storage::use_byte_size_t{}, 8, GetCPUAllocator());
    REQUIRE(s.UseCount() == 1);
    REQUIRE(s.Unique());
}

TEST_CASE("Copying a Storage shares the same StorageImpl and bumps the refcount", "[Storage][refcount]") {
    Storage a(Storage::use_byte_size_t{}, 8, GetCPUAllocator());
    Storage b = a; // copy

    REQUIRE(a.UseCount() == 2);
    REQUIRE(b.UseCount() == 2);
    REQUIRE_FALSE(a.Unique());
    REQUIRE_FALSE(b.Unique());

    // Same underlying buffer, not a deep copy.
    REQUIRE(a.Data() == b.Data());
}

TEST_CASE("A write through one Storage copy is visible through the other", "[Storage][refcount]") {
    Storage a(Storage::use_byte_size_t{}, 8, GetCPUAllocator());
    Storage b = a;

    std::memset(a.MutableData(), 0x7F, 8);
    REQUIRE(std::memcmp(a.Data(), b.Data(), 8) == 0);
}

TEST_CASE("Dropping one copy leaves the other Storage unique again", "[Storage][refcount]") {
    Storage a(Storage::use_byte_size_t{}, 8, GetCPUAllocator());
    {
        Storage b = a;
        REQUIRE(a.UseCount() == 2);
    } // b destroyed here
    REQUIRE(a.UseCount() == 1);
    REQUIRE(a.Unique());
}

TEST_CASE("Move-constructing a Storage transfers ownership without bumping refcount", "[Storage][refcount]") {
    Storage a(Storage::use_byte_size_t{}, 8, GetCPUAllocator());
    const void* original_data = a.Data();

    Storage b = std::move(a);

    REQUIRE(b.UseCount() == 1);
    REQUIRE(b.Unique());
    REQUIRE(b.Data() == original_data);
    // `a` is moved-from: IntrusivePtr's move ctor nulls out the source target,
    // so `a` should now be falsy rather than a dangling/aliasing handle.
    REQUIRE_FALSE(static_cast<bool>(a));
}

TEST_CASE("Storage can be built directly from an IntrusivePtr<StorageImpl>", "[Storage]") {
    auto impl = MakeIntrusive<StorageImpl>(
        StorageImpl::use_byte_size_t{}, 24, GetCPUAllocator(), false);
    Storage s(std::move(impl));

    REQUIRE(s.SizeBytes() == 24);
    REQUIRE(s.UseCount() == 1);
}

TEST_CASE("StorageImpl::Reset clears the data pointer and zeroes size", "[StorageImpl]") {
    StorageImpl impl(StorageImpl::use_byte_size_t{}, 16, GetCPUAllocator(), false);
    REQUIRE(impl.SizeBytes() == 16);
    REQUIRE(impl.Data() != nullptr);

    impl.Reset();

    REQUIRE(impl.SizeBytes() == 0);
    REQUIRE(impl.Data() == nullptr);
}

// StorageImpl direct construction (bypassing Storage)
TEST_CASE("StorageImpl allocating ctor allocates real, writable memory", "[StorageImpl]") {
    StorageImpl impl(StorageImpl::use_byte_size_t{}, 32, GetCPUAllocator(), false);
    REQUIRE(impl.SizeBytes() == 32);
    REQUIRE(impl.MutableData() != nullptr);

    std::memset(impl.MutableData(), 0x11, 32);
    auto* bytes = static_cast<unsigned char*>(impl.MutableData());
    for (std::size_t i = 0; i < 32; ++i) {
        REQUIRE(bytes[i] == 0x11);
    }
}

TEST_CASE("StorageImpl DataPtr ctor stores the exact DataPtr passed in", "[StorageImpl]") {
    DataPtr dp = GetCPUAllocator()->Allocate(10);
    void* raw = dp.Get();

    StorageImpl impl(StorageImpl::use_byte_size_t{}, 10, std::move(dp), GetCPUAllocator(), false);

    REQUIRE(impl.Data() == raw);
    REQUIRE(impl.DataPtr().Get() == raw);
}
