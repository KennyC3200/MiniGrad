#pragma once

#include "IntrusivePtr.hpp"
#include "Allocator.hpp"
#include "Device.hpp"
#include "DeviceType.hpp"
#include <cstdint>

namespace mg {

struct StorageImpl : public IntrusivePtrTarget {
public:
    struct use_byte_size_t {};
    
    // Create a storage impl, allocating `size_bytes` on the given allocator
    StorageImpl(
        use_byte_size_t,
        std::size_t size_bytes,
        Allocator* allocator,
        bool resizable)
        : m_size_bytes(size_bytes)
        , m_data_ptr(allocator->Allocate(size_bytes))
        , m_allocator(allocator)
        , m_resizable(resizable) {}

    // Constructor for when the allocation has already been made 
    // hence the data_ptr to that allocation
    StorageImpl(
        use_byte_size_t,
        std::size_t size_bytes,
        DataPtr data_ptr,
        Allocator* allocator,
        bool resizable)
        : m_size_bytes(size_bytes)
        , m_data_ptr(std::move(data_ptr))
        , m_allocator(allocator)
        , m_resizable(resizable) {}

    StorageImpl() = delete;
    StorageImpl(StorageImpl&& rhs) = delete;
    StorageImpl(const StorageImpl& rhs) = delete;
    StorageImpl& operator=(StorageImpl&& other) = delete;
    StorageImpl& operator=(const StorageImpl& other) = delete;
    ~StorageImpl() override = default;

    void Reset() {
        m_data_ptr.Clear();
        m_size_bytes = 0;
    }

    std::size_t SizeBytes() const { return m_size_bytes; }
    const void* Data() const { return m_data_ptr.Get(); }
    void* MutableData() { return m_data_ptr.MutableGet(); }
    bool Resizable() const { return m_resizable; }
    mg::Allocator* Allocator() const { return m_allocator; }
    mg::DeviceType DeviceType() const { return m_data_ptr.Device().GetType(); }
    mg::Device Device() const { return m_data_ptr.Device(); }
    const mg::DataPtr& DataPtr() const { return m_data_ptr; }

    void ReleaseResources() override { m_data_ptr.Clear(); }

private:
    std::size_t m_size_bytes;
    mg::DataPtr m_data_ptr;
    mg::Allocator* m_allocator;
    bool m_resizable;
};

}