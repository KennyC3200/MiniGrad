#include "Allocator.hpp"
#include <array>
#include <cstring>

namespace mg {

// Global allocators
static std::array<Allocator*, mg::NUM_DEVICE_TYPES> g_allocators{};

DataPtr Allocator::Clone(const void* data, std::size_t n) {
    DataPtr new_data = Allocate(n);
    CopyData(new_data.MutableGet(), data, n);
    return new_data;
}

bool Allocator::IsSimpleDataPtr(const DataPtr& data_ptr) const {
    return data_ptr.Get() == data_ptr.GetContext();
}

void Allocator::DefaultCopyData(void* dest, const void* src, std::size_t cnt) const {
    std::memcpy(dest, src, cnt);
}

void SetAllocator(DeviceType type, Allocator* alloc) {
    g_allocators[static_cast<int>(type)] = alloc;
}

Allocator* GetAllocator(const DeviceType& type) {
    auto* alloc = g_allocators[static_cast<int>(type)];
    INTERNAL_ASSERT_DEBUG_ONLY(alloc, "Allocator {} didn't get set", DeviceTypeName(type));
    return alloc;
}

}