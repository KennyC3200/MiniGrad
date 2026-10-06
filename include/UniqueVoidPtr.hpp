#pragma once

#include <memory>

namespace mg {

using DeleterFnPtr = void (*)(void*);

// Doesn't delete anything
void DeleteNothing(void* /* unused */);

class UniqueVoidPtr {
public:
    // Constructors
    UniqueVoidPtr() : m_data(nullptr), m_ctx(nullptr, &DeleteNothing) {}
    explicit UniqueVoidPtr(void* data) : m_data(data), m_ctx(nullptr, &DeleteNothing) {}
    UniqueVoidPtr(void* data, void* ctx, DeleterFnPtr ctx_deleter)
        : m_data(data), m_ctx(ctx, ctx_deleter ? ctx_deleter : &DeleteNothing) {}

    // Destructor
    ~UniqueVoidPtr() noexcept = default;

    // Copy constructor
    UniqueVoidPtr(const UniqueVoidPtr&) = delete;

    // Copy assignment
    UniqueVoidPtr& operator=(const UniqueVoidPtr&) = delete;

    // Move constructor
    UniqueVoidPtr(UniqueVoidPtr&& rhs) noexcept
        : m_data(rhs.m_data), m_ctx(std::move(rhs.m_ctx)) { rhs.m_data = nullptr; }

    // Move assignment
    UniqueVoidPtr& operator=(UniqueVoidPtr&& rhs) & noexcept {
        m_data = rhs.m_data;
        m_ctx = std::move(rhs.m_ctx);

        rhs.m_data = nullptr;

        return *this;
    }

    void Clear() { m_data = nullptr; m_ctx = nullptr; }
    [[nodiscard]] void* Get() const { return m_data; }
    [[nodiscard]] void* GetContext() const { return m_ctx.get(); }
    [[nodiscard]] void* ReleaseContext() { return m_ctx.release(); }
    [[nodiscard]] DeleterFnPtr GetDeleter() const { return m_ctx.get_deleter(); }

    template<typename T>
    [[nodiscard]] T* CastContext(DeleterFnPtr expected_deleter) const {
        if (GetDeleter() != expected_deleter) return nullptr;
        return static_cast<T*>(GetContext());
    }

    [[nodiscard]] operator bool() const { return m_data || m_ctx; }
    [[nodiscard]] void* operator->() const { return m_data; }

private:
    void* m_data;
    std::unique_ptr<void, DeleterFnPtr> m_ctx;
};

[[nodiscard]] inline bool operator==(const UniqueVoidPtr& sp, std::nullptr_t) noexcept {
    return !sp;
}

[[nodiscard]] inline bool operator==(std::nullptr_t, const UniqueVoidPtr& sp) noexcept {
    return !sp;
}

[[nodiscard]] inline bool operator!=(const UniqueVoidPtr& sp, std::nullptr_t) noexcept {
    return sp;
}

[[nodiscard]] inline bool operator!=(std::nullptr_t, const UniqueVoidPtr& sp) noexcept {
    return sp;
}

}