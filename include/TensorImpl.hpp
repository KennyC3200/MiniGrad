#pragma once

#include "Device.hpp"
#include "IntrusivePtr.hpp"
#include "Storage.hpp"
#include "SizesAndStrides.hpp"

namespace mg {

enum class DType {
    Float32,
    Float64,
    Int32,
    Int64
};

struct TensorImpl : public IntrusivePtrTarget {
public:
    TensorImpl() = delete;
    ~TensorImpl() override = default;
    TensorImpl(Storage&& storage, DType dtype, Device device) 
        : m_storage(std::move(storage)), m_dtype(dtype), m_device(device) {}

    // Sizes and strides
    SizesAndStrides GetSizesAndStrides() { return m_sizes_and_strides; }
    std::size_t Rank() const noexcept { return m_sizes_and_strides.Rank(); }
    std::size_t SizeAt(std::size_t size_idx) const noexcept {
        return m_sizes_and_strides.SizeAt(size_idx);
    }

    std::size_t& SizeAt(std::size_t size_idx) noexcept { 
        return m_sizes_and_strides.SizeAt(size_idx);
    }

    SizesAndStrides::SizesIterator SizesBegin() noexcept {
        return m_sizes_and_strides.SizesBegin();
    }

    SizesAndStrides::SizesIterator SizesEnd() noexcept {
        return m_sizes_and_strides.SizesEnd();
    }

    SizesAndStrides::SizesConstIterator SizesBegin() const noexcept {
        return m_sizes_and_strides.SizesBegin();
    }

    SizesAndStrides::SizesConstIterator SizesEnd() const noexcept {
        return m_sizes_and_strides.SizesEnd();
    }

    std::size_t StrideAt(std::size_t stride_idx) const noexcept { 
        return m_sizes_and_strides.StrideAt(stride_idx);
    }

    std::size_t& StrideAt(std::size_t stride_idx) noexcept { 
        return m_sizes_and_strides.StrideAt(stride_idx);
    }

    SizesAndStrides::StridesIterator StridesBegin() noexcept {
        return m_sizes_and_strides.StridesBegin();
    }

    SizesAndStrides::StridesIterator StridesEnd() noexcept {
        return m_sizes_and_strides.StridesEnd();
    }

    SizesAndStrides::StridesConstIterator StridesBegin() const noexcept {
        return m_sizes_and_strides.StridesBegin();
    }

    SizesAndStrides::StridesConstIterator StridesEnd() const noexcept {
        return m_sizes_and_strides.StridesEnd();
    }

    void Resize(std::size_t new_size) {
        m_sizes_and_strides.Resize(new_size);
    }

    std::size_t StorageOffset() const { return m_storage_offset; }
    mg::DeviceType DeviceType() const { return m_device.GetType(); }
    Device GetDevice() const { return m_device; }
    DType GetDType() const { return m_dtype; }

protected:
    Storage m_storage;
    SizesAndStrides m_sizes_and_strides;

    std::size_t m_storage_offset = 0;

    DType m_dtype;
    Device m_device;
};

}