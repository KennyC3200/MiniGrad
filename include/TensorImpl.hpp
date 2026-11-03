#pragma once

#include "Device.hpp"
#include "IntrusivePtr.hpp"
#include "Storage.hpp"
#include "SizesAndStrides.hpp"

#include <span>
#include <cstdint>
#include <memory>

namespace mg {

struct AutogradMetadata;
class TensorBase;
class Tensor;

enum class DType {
    Float32,
    Float64,
    Int32,
    Int64
};

struct TensorImpl : public IntrusivePtrTarget {
public:
    TensorImpl() = delete;
    ~TensorImpl() override;
    TensorImpl(Storage&& storage, DType dtype, Device device);

    const SizesAndStrides& GetSizesAndStrides() const { return m_sizes_and_strides; }

    std::size_t Rank() const noexcept { return m_sizes_and_strides.Rank(); }

    std::size_t SizeAt(std::size_t size_idx) const noexcept {
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

    void Resize(std::size_t new_rank);

    void SetSizesContiguous(std::span<const std::size_t> sizes);

    void SetSizesAndStrides(std::span<const std::size_t> sizes,
                            std::span<const std::size_t> strides);

    void SetStorageOffset(std::size_t offset) { m_storage_offset = offset; }

    std::size_t StorageOffset() const { return m_storage_offset; }

    mg::DeviceType DeviceType() const { return m_device.GetType(); }

    DType GetDType() const { return m_dtype; }

    Device GetDevice() const { return m_device; }

    std::span<const std::size_t> Sizes() const { 
        return std::span<const std::size_t>(
            m_sizes_and_strides.SizesBegin(), 
            m_sizes_and_strides.SizesEnd());
    }

    std::span<const std::size_t> Strides() const {
        return std::span<const std::size_t>(
            m_sizes_and_strides.StridesBegin(),
            m_sizes_and_strides.StridesEnd());
    }

    std::size_t Numel() const { return m_numel; }

    bool IsContiguous() const { return m_contiguous; }

    std::size_t ItemSize() const;

    const void* Data() const {
        return static_cast<const std::uint8_t*>(m_storage.Data()) + StorageOffset() * ItemSize();
    }

    void* MutableData() { 
        return static_cast<std::uint8_t*>(m_storage.MutableData()) + StorageOffset() * ItemSize(); 
    }

    bool RequiresGrad() const;

    void SetRequiresGrad(bool requires_grad);

    const Tensor& Grad() const;

    Tensor& MutableGrad();

    IntrusivePtr<TensorImpl> ShallowCopyAndDetach() const;

private:
    Storage m_storage;
    SizesAndStrides m_sizes_and_strides;

    // Starting index, measured in number of elements and NOT bytes
    std::size_t m_storage_offset = 0;

    DType m_dtype;
    Device m_device;

    // Caches the numel. Should be called at the end of every size change
    void RefreshNumel() {
        m_numel = ComputeNumel();
    }

    // Returns a numel that accounts for overflow
    std::size_t ComputeNumel() const;

    // Cache the number of elements b/c it is constantly being used
    std::size_t m_numel = 0;

    // Cache if the tensor is contiguous or not
    bool m_contiguous = false;

    void RefreshContiguous() { m_contiguous = ComputeContiguous(); }

    bool ComputeContiguous() const;

    void RestrideContiguous();

    std::unique_ptr<AutogradMetadata> m_autograd_metadata;
};

}