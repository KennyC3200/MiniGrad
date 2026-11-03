#include "TensorImpl.hpp"
#include "AutogradMetadata.hpp"
#include "IntrusivePtr.hpp"

#include <algorithm>

namespace mg {

// AutogradMetadata is forward declared and is incomplete. However, std::unique_ptr's deleter needs
// a full type
TensorImpl::~TensorImpl() = default;

TensorImpl::TensorImpl(Storage&& storage, DType dtype, Device device) 
    : m_storage(std::move(storage)), m_dtype(dtype), m_device(device) {}

std::size_t TensorImpl::ItemSize() const {
    switch (m_dtype) {
        case mg::DType::Int32: return 4;
        case mg::DType::Int64: return 8;
        case mg::DType::Float32: return 4;
        case mg::DType::Float64: return 8;
    }
}

void TensorImpl::Resize(std::size_t new_rank) {
    m_sizes_and_strides.Resize(new_rank);
    RefreshNumel();
    RefreshContiguous();
}

void TensorImpl::SetSizesContiguous(std::span<const std::size_t> sizes) {
    // Set the sizes contiguously. The stride is automatically calculated
    ASSERT(sizes.size() <= MINIGRAD_TENSOR_MAX_RANK,
        "Rank {} exceeds max rank {}", sizes.size(), MINIGRAD_TENSOR_MAX_RANK);

    m_sizes_and_strides.SetSizes(sizes);

    RefreshNumel();
    RestrideContiguous();
}

void TensorImpl::SetSizesAndStrides(std::span<const std::size_t> sizes,
                        std::span<const std::size_t> strides) 
{
    ASSERT(sizes.size() <= MINIGRAD_TENSOR_MAX_RANK,
        "Rank of sizes {} exceeds max rank {}", sizes.size(), MINIGRAD_TENSOR_MAX_RANK);
    ASSERT(strides.size() <= MINIGRAD_TENSOR_MAX_RANK,
        "Rank of strides {} exceeds max rank {}", strides.size(), MINIGRAD_TENSOR_MAX_RANK);
    ASSERT(sizes.size() == strides.size(), 
        "Rank of sizes {} must match rank of strides {}", sizes.size(), strides.size());

    m_sizes_and_strides.SetSizes(sizes);
    std::copy(strides.begin(), strides.end(), StridesBegin());

    RefreshNumel();

    // Need to refresh contiguous depending on the user's sizes and strides
    RefreshContiguous();
}

bool TensorImpl::ComputeContiguous() const {
    // Mathematically, it is contiguous if stride[i] = stride[i + 1] * shape[i + 1]
    // Work from the inside out, calculating the expected stride each step
    if (m_numel == 0) return true;

    std::size_t expected_stride = 1;
    for (std::size_t i = Rank(); i-- > 0;) {
        const std::size_t size = SizeAt(i);
        if (size == 1) continue;
        if (StrideAt(i) != expected_stride) return false;
        expected_stride *= size;
    }

    return true;
}

void TensorImpl::RestrideContiguous() {
    // Size is clamped to 1 to handle zero-dimensions, such as shape (0, 3) to
    // prevent every outer stride from becoming 0
    std::size_t stride = 1;
    for (std::size_t i = Rank(); i-- > 0;) {
        m_sizes_and_strides.StrideAt(i) = stride;

        stride *= std::max<std::size_t>(SizeAt(i), 1);
    }

    m_contiguous = true;
}

std::size_t TensorImpl::ComputeNumel() const {
    // Returns a numel that accounts for overflow
    std::size_t numel = 1;
    for (std::size_t size : Sizes()) {
        if (size == 0) return 0;
        ASSERT(numel <= SIZE_MAX / size, "numel: multiplication overflow");
        numel *= size;
    }
    return numel;
}

bool TensorImpl::RequiresGrad() const {
    return m_autograd_metadata && m_autograd_metadata->m_required_grad;
}

void TensorImpl::SetRequiresGrad(bool requires_grad) {
    if (!requires_grad && !m_autograd_metadata) return;
    if (!m_autograd_metadata) m_autograd_metadata = std::make_unique<AutogradMetadata>();
    m_autograd_metadata->m_required_grad = requires_grad;
}

const Tensor& TensorImpl::Grad() const {
    // Grad() must return a ref even with no metadata
    static const Tensor undef;
    return m_autograd_metadata ? m_autograd_metadata->m_grad : undef;
}

Tensor& TensorImpl::MutableGrad() {
    // Create a new m_autograd_metadata since the user can modify the returned mutable reference
    if (!m_autograd_metadata) m_autograd_metadata = std::make_unique<AutogradMetadata>();
    return m_autograd_metadata->m_grad;
}

// Copies the storage, sizes and strides, etc. but the copy is detached from the autograd graph,
// meaning gradients can't flow back through it. Therefore, the m_autograd_metadata is not copied
IntrusivePtr<TensorImpl> TensorImpl::ShallowCopyAndDetach() const {
    auto copy = MakeIntrusive<TensorImpl>(Storage(m_storage), m_dtype, m_device);

    copy->m_storage_offset = m_storage_offset;
    copy->m_numel = m_numel;
    copy->m_contiguous = m_contiguous;
    copy->m_sizes_and_strides = m_sizes_and_strides;
    copy->m_autograd_metadata = nullptr;

    return copy;
}

}