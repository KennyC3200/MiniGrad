#include "TensorImpl.hpp"
#include "CPUAllocator.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace mg;

namespace {

IntrusivePtr<TensorImpl> MakeEmptyImpl() {
    return MakeIntrusive<TensorImpl>(
        Storage(Storage::use_byte_size_t{}, 0, GetCPUAllocator()),
        DType::Float32,
        Device(DeviceType::CPU));
}

}

TEST_CASE("Newly constructed TensorImpl is a 1D tensor with no elements", "[TensorImpl][numel]") {
    auto impl = MakeEmptyImpl();
    REQUIRE(impl->Rank() == 1);
    REQUIRE(impl->SizeAt(0) == 0);
    REQUIRE(impl->Numel() == 0);
}

TEST_CASE("Numel agrees with the product of Sizes() after construction", "[TensorImpl][numel]") {
    // Guards against the cache's initial value drifting from the default shape
    auto impl = MakeEmptyImpl();
    std::size_t product = 1;
    for (std::size_t size : impl->Sizes()) product *= size;
    REQUIRE(impl->Numel() == product);
}

TEST_CASE("Rank 0 (scalar) has numel 1, the empty product", "[TensorImpl][numel]") {
    auto impl = MakeEmptyImpl();
    impl->Resize(0);
    REQUIRE(impl->Rank() == 0);
    REQUIRE(impl->Sizes().empty());
    REQUIRE(impl->Numel() == 1);
}

TEST_CASE(
    "Growing the rank refreshes numel; new dims are zeroed so numel is 0", 
    "[TensorImpl][numel]") 
{
    auto impl = MakeEmptyImpl();
    impl->Resize(0);
    REQUIRE(impl->Numel() == 1);

    impl->Resize(3);
    REQUIRE(impl->Rank() == 3);
    REQUIRE(impl->Numel() == 0);
}

TEST_CASE("Resizing to the same rank keeps numel unchanged", "[TensorImpl][numel]") {
    auto impl = MakeEmptyImpl();
    impl->Resize(0);
    impl->Resize(0);
    REQUIRE(impl->Numel() == 1);
}

TEST_CASE("dim-0 tensor bug should yield false contiguous", "[TensorImpl][contiguous]") {
    auto impl = MakeEmptyImpl();
    // This method doesn't check for contiguous
    impl->SetSizesAndStrides(std::vector<std::size_t>{2, 3}, std::vector<std::size_t>{6, 1});
    REQUIRE(impl->IsContiguous() == false);
}

TEST_CASE(
    "rank-0 sizes and strides should given numel of 1 and is contiguous", 
    "[TensorImpl][numel][contiguous]")
{
    auto impl = MakeEmptyImpl();
    impl->SetSizesAndStrides({}, {});
    REQUIRE(impl->Numel() == 1);
    REQUIRE(impl->IsContiguous());
}

TEST_CASE(
    "Setting sizes contiguous (0, 3) gives correct strides of (3, 1)", 
    "[TensorImpl][strides]")
{
    auto impl = MakeEmptyImpl();
    impl->SetSizesContiguous(std::vector<std::size_t>{0, 3});
    REQUIRE(std::ranges::equal(impl->Strides(), std::vector<std::size_t>{3, 1}));
}