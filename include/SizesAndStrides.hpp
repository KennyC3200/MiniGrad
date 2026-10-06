#pragma once

#include "Exception.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace mg {

constexpr std::size_t MINIGRAD_TENSOR_MAX_RANK = 3;

class SizesAndStrides {
public:
    using SizesIterator = std::size_t*;
    using SizesConstIterator = const std::size_t*;
    using StridesIterator = std::size_t*;
    using StridesConstIterator = const std::size_t*;

    SizesAndStrides() {
        SizeAt(0) = 0;
        StrideAt(0) = 1;
    }

    SizesAndStrides(const SizesAndStrides& rhs) : m_rank(rhs.m_rank) {
        CopyData(rhs);
    }

    // Move from rhs, so rhs.m_rank == 0 after
    SizesAndStrides(SizesAndStrides&& rhs) noexcept : m_rank(rhs.m_rank) {
        memcpy(m_storage, rhs.m_storage, sizeof(m_storage));
        rhs.m_rank = 0;
    }

    bool operator==(const SizesAndStrides& rhs) const {
        if (m_rank != rhs.m_rank) return false;
        return (std::memcmp(m_storage, rhs.m_storage, sizeof(m_storage)) == 0);
    }

    bool operator!=(const SizesAndStrides& rhs) const {
        return !(*this == rhs);
    }

    [[nodiscard]] std::size_t Rank() const noexcept { return m_rank; }

    SizesAndStrides& operator=(const SizesAndStrides& rhs) {
        if (this == &rhs) return *this;
        CopyData(rhs);
        m_rank = rhs.m_rank;
        return *this;
    }

    SizesAndStrides& operator=(SizesAndStrides&& rhs) noexcept {
        if (this == &rhs) return *this;
        CopyData(rhs);
        m_rank = rhs.m_rank;
        rhs.m_rank = 0;
        return *this;
    }

    std::size_t SizeAt(std::size_t size_idx) const noexcept { 
        InternalAssertSizeAndStride(size_idx);
        return m_storage[size_idx];
    }

    std::size_t& SizeAt(std::size_t size_idx) noexcept { 
        InternalAssertSizeAndStride(size_idx);
        return m_storage[size_idx]; 
    }

    SizesIterator SizesBegin() noexcept {
        return &m_storage[0];
    }

    SizesIterator SizesEnd() noexcept {
        return &m_storage[m_rank];
    }

    SizesConstIterator SizesBegin() const noexcept {
        return &m_storage[0];
    }

    SizesConstIterator SizesEnd() const noexcept {
        return &m_storage[m_rank];
    }

    std::size_t StrideAt(std::size_t stride_idx) const noexcept { 
        InternalAssertSizeAndStride(stride_idx);
        return m_storage[stride_idx + MINIGRAD_TENSOR_MAX_RANK]; 
    }

    std::size_t& StrideAt(std::size_t stride_idx) noexcept { 
        InternalAssertSizeAndStride(stride_idx);
        return m_storage[stride_idx + MINIGRAD_TENSOR_MAX_RANK]; 
    }

    StridesIterator StridesBegin() noexcept {
        return &m_storage[MINIGRAD_TENSOR_MAX_RANK];
    }

    StridesIterator StridesEnd() noexcept {
        return &m_storage[MINIGRAD_TENSOR_MAX_RANK + m_rank];
    }

    StridesConstIterator StridesBegin() const noexcept {
        return &m_storage[MINIGRAD_TENSOR_MAX_RANK];
    }

    StridesConstIterator StridesEnd() const noexcept {
        return &m_storage[MINIGRAD_TENSOR_MAX_RANK + m_rank];
    }

    void Resize(std::size_t new_rank) {
        const std::size_t prev_rank = m_rank;
        if (new_rank == prev_rank) return;

        // Ensure new_rank is within MINIGRAD_TENSOR_MAX_RANK
        InternalAssertRank(new_rank);

        // Calculate the bytes in m_storage array to zero
        std::size_t rank_diff = new_rank > prev_rank
            ? new_rank - prev_rank 
            : prev_rank - new_rank;
        std::size_t bytes_to_zero = rank_diff * sizeof(m_storage[0]);

        if (new_rank > prev_rank) {
            // Zero newly added sizes and strides
            memset(&m_storage[prev_rank], 0, bytes_to_zero);
            memset(&m_storage[MINIGRAD_TENSOR_MAX_RANK + prev_rank], 0, bytes_to_zero);
        } else {
            // Zero previously occupied sizes and strides
            memset(&m_storage[new_rank], 0, bytes_to_zero);
            memset(&m_storage[MINIGRAD_TENSOR_MAX_RANK + new_rank], 0, bytes_to_zero);
        }

        m_rank = new_rank;
    }

private:
    void InternalAssertSizeAndStride(std::size_t idx) const {
#ifdef MINIGRAD_DEBUG
        INTERNAL_ASSERT_DEBUG_ONLY(idx < m_rank);
#endif
    };

    void InternalAssertRank(std::size_t rank) const {
#ifdef MINIGRAD_DEBUG
        INTERNAL_ASSERT_DEBUG_ONLY(rank <= MINIGRAD_TENSOR_MAX_RANK);
#endif
    }

    void CopyData(const SizesAndStrides& rhs) {
        memcpy(m_storage, rhs.m_storage, sizeof(m_storage));
    }

    std::size_t m_rank{1};

    // Packed storage for the sizes and strides
    // Half is reserved for sizes, and other half is for strides
    std::size_t m_storage[MINIGRAD_TENSOR_MAX_RANK * 2]{};
};

}