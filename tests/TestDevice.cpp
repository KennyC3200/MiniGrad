#include "Device.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace mg;

TEST_CASE("Device stores the type it was constructed with", "[Device]") {
    Device d(DeviceType::CUDA, 0);
    REQUIRE(d.GetType() == DeviceType::CUDA);
    REQUIRE(d.IsCUDA());
    REQUIRE_FALSE(d.IsCPU());
}

TEST_CASE("Device equality compares both type and index", "[Device]") {
    Device a(DeviceType::CUDA, 0);
    Device b(DeviceType::CUDA, 0);
    Device c(DeviceType::CUDA, 1);
    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
}

TEST_CASE("Device idx defaults to -1 (no index) and HasIdx reflects that", "[Device]") {
    Device d(DeviceType::CUDA);
    REQUIRE(d.Idx() == -1);
    REQUIRE_FALSE(d.HasIdx());
}

TEST_CASE("SetIdx updates the index", "[Device]") {
    Device d(DeviceType::CUDA, 0);
    d.SetIdx(2);
    REQUIRE(d.Idx() == 2);
    REQUIRE(d.HasIdx());
}

TEST_CASE("std::hash<Device> distinguishes different devices", "[Device][hash]") {
    Device a(DeviceType::CUDA, 0);
    Device b(DeviceType::CUDA, 1);
    std::hash<Device> hasher;
    // Not a strict guarantee in general (hash collisions are legal), but for
    // this specific bit-packing scheme adjacent indices must hash differently.
    REQUIRE(hasher(a) != hasher(b));
}

// MINIGRAD_DEBUG
// Device::Validate() only runs under MINIGRAD_DEBUG. These are written so
// that if/when that macro is enabled during a test build, they immediately
// tell you whether the CPU-index check is doing what it's supposed to.
#ifdef MINIGRAD_DEBUG

TEST_CASE("Constructing a CPU device with idx -1 should be valid", "[Device][debug]") {
    // This is the ordinary, unqualified way to build a CPU device and is
    // used throughout the codebase (e.g. every CPU allocation). It must not
    // throw under MINIGRAD_DEBUG.
    REQUIRE_NOTHROW(Device(DeviceType::CPU));
}

TEST_CASE("Constructing a CPU device with idx 0 should be valid", "[Device][debug]") {
    REQUIRE_NOTHROW(Device(DeviceType::CPU, 0));
}

#endif
