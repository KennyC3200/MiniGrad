#pragma once

#include "StorageImpl.hpp"
#include "IntrusivePtr.hpp"

namespace mg {

struct Storage {
public:
    struct use_byte_size_t {};

    Storage() = default;
    Storage(IntrusivePtr<StorageImpl> ptr) : m_storage_impl(std::move(ptr)) {}

    // Allocates `size_bytes` memory buffer using the given allocator and creates a storage
    Storage(
        use_byte_size_t, 
        std::size_t size_bytes, 
        Allocator* allocator, 
        bool resizable = false) 
        : m_storage_impl(MakeIntrusive<StorageImpl>(
            StorageImpl::use_byte_size_t(),
            size_bytes,
            allocator,
            resizable)) {}
    
    // Constructor for when the allocation has already been made 
    // hence the data_ptr to that allocation
    Storage(
        use_byte_size_t,
        std::size_t size_bytes,
        DataPtr data_ptr,
        Allocator* allocator,
        bool resizable = false
    ) : m_storage_impl(MakeIntrusive<StorageImpl>(
        StorageImpl::use_byte_size_t(),
        size_bytes,
        std::move(data_ptr),
        allocator,
        resizable)) {}
    
    std::size_t SizeBytes() const { return m_storage_impl->SizeBytes(); }
    const void* Data() const { return m_storage_impl->Data(); }
    void* MutableData() { return m_storage_impl->MutableData(); }
    bool Resizable() const { return m_storage_impl->Resizable(); }
    mg::Allocator* Allocator() const { return m_storage_impl->Allocator(); }
    mg::DeviceType DeviceType() const { return m_storage_impl->Device().GetType(); }
    mg::Device Device() const { return m_storage_impl->Device(); }
    const mg::DataPtr& DataPtr() const { return m_storage_impl->DataPtr(); }
    operator bool() const { return m_storage_impl.Defined(); }
    std::size_t UseCount() const { return m_storage_impl.UseCount(); }
    inline bool Unique() const { return m_storage_impl.Unique(); }

protected:
    IntrusivePtr<StorageImpl> m_storage_impl;
};

}