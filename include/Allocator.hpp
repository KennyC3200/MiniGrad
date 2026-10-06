#pragma once

#include "Exception.hpp"
#include "UniqueVoidPtr.hpp"
#include "Device.hpp"
#include "DeviceType.hpp"

namespace mg {

class DataPtr {
public:
    DataPtr() : m_device(DeviceType::CPU) {}
    DataPtr(void* data, Device device) : m_ptr(data), m_device(device) {}
    DataPtr(void* data, void* ctx, DeleterFnPtr ctx_deleter, Device device)
        : m_ptr(data, ctx, ctx_deleter), m_device(device) {}
    void Clear() { m_ptr.Clear(); }
    void* Get() const { return m_ptr.Get(); }
    void* MutableGet() { return m_ptr.Get(); }
    void* GetContext() const { return m_ptr.GetContext(); }
    void* ReleaseContext() { return m_ptr.ReleaseContext(); }
    operator bool() const { return static_cast<bool>(m_ptr); }

    template<typename T>
    T* CastContext(DeleterFnPtr expected_deleter) const {
        return m_ptr.CastContext<T>(expected_deleter);
    }

    DeleterFnPtr GetDeleter() { return m_ptr.GetDeleter(); }
    DeleterFnPtr GetDeleter() const { return m_ptr.GetDeleter(); }

    mg::Device Device() const { return m_device; }

private:
    mg::UniqueVoidPtr m_ptr;
    mg::Device m_device;
};

[[nodiscard]] inline bool operator==(const DataPtr& ptr, std::nullptr_t) noexcept {
    return !ptr;
}

[[nodiscard]] inline bool operator==(std::nullptr_t, const DataPtr& ptr) noexcept {
    return !ptr;
}

[[nodiscard]] inline bool operator!=(const DataPtr& ptr, std::nullptr_t) noexcept {
    return ptr;
}

[[nodiscard]] inline bool operator!=(std::nullptr_t, const DataPtr& ptr) noexcept {
    return ptr;
}

struct Allocator {
public:
    virtual ~Allocator() = default;

    virtual DataPtr Allocate(std::size_t n) = 0;

    virtual DeleterFnPtr RawDeleter() const { return nullptr; }

    // Clones an allocation that came from this allocator
    // To perform this, it calls CopyData, which must be implemented by the derived class
    // Requires: input data was allocated by the same allocator
    DataPtr Clone(const void* data, std::size_t n);

    // Checks if DataPtr has simple context--not warapped with any out of the ordinary contexts
    virtual bool IsSimpleDataPtr(const DataPtr& data_ptr) const;

    void* RawAllocate(std::size_t n) {
        auto dp = Allocate(n);
        ASSERT(
            dp.Get() == dp.GetContext(), 
            "DataPtr address does not equal DataPtr context address");
        return dp.ReleaseContext();
    }

    void RawDeallocate(void* ptr) {
        auto d = RawDeleter();
        ASSERT(d, "Deleter for DataPtr does not exist");
        d(ptr);
    }

    // Copies data from one allocation to another
    // Pure virtual
    // Derived class implementation can simply call `DefaultCopyData`
    // Requires: src and dest were allocated by this allocator
    // Requires: src and dest both have length >= cnt
    virtual void CopyData(void* dest, const void* src, std::size_t cnt) const = 0;

protected:
    // Uses std::memcpy to copy data
    void DefaultCopyData(void* dest, const void* src, std::size_t cnt) const;
};

// Set the allocator for a DeviceType `type`. The passed in allocator pointer is expected to
// have a static lifetime; this function does NOT take ownership of the raw pointer
// Also note that this is not thread-safe--we assume that this function will only be called during
// initialization.
void SetAllocator(DeviceType type, Allocator* alloc);
Allocator* GetAllocator(const DeviceType& type);

template <DeviceType T>
struct AllocatorRegistar {
    explicit AllocatorRegistar(Allocator* alloc) {
        SetAllocator(T, alloc);
    }
};

#define REGISTER_ALLOCATOR(type, allocator)                     \
namespace {                                                     \
    static AllocatorRegistar<type> g_alloc_registry(allocator); \
}

}