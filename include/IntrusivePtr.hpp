#pragma once

#include <atomic>
#include <type_traits>
#include <cstdint>
#include <utility>
#include <cstddef>
#include <memory>

namespace mg {

struct DontIncreaseRefCount {};

constexpr uint64_t REFCOUNT_ONE = 1;
constexpr uint64_t WEAKCOUNT_ONE = REFCOUNT_ONE << 32;
constexpr uint64_t UNIQUE_REF = REFCOUNT_ONE | WEAKCOUNT_ONE;

template <class TargetType, class ToNullType, class FromNullType>
TargetType* AssignPtr(TargetType* rhs) {
    if (FromNullType::Singleton() == rhs) return ToNullType::Singleton();
    return rhs;
}

template <class TargetType>
struct IntrusiveTargetDefaultNullType final {
    static constexpr TargetType* Singleton() noexcept { return nullptr; }
};

inline uint32_t Refcount(uint64_t combined_refcount) {
    return static_cast<uint32_t>(combined_refcount);
}

inline uint32_t Weakcount(uint64_t combined_refcount) {
    return static_cast<uint32_t>(combined_refcount >> 32);
}

inline bool IsUniquelyOwned(uint64_t combined_refcount) {
    return combined_refcount == UNIQUE_REF;
}

// An increment only needs to change the counter safely. A decrement can potentially be the
// operator that discovers it is the last owner, and therefore needs to synchronize with everything
// the other owners did before they released their references
inline uint64_t AtomicCombinedRefcountIncrement(
    std::atomic<uint64_t>& combined_refcount, uint64_t increment
) {
    return combined_refcount.fetch_add(increment, std::memory_order_relaxed) + increment;
}

inline uint64_t AtomicWeakcountIncrement(std::atomic<uint64_t>& combined_refcount) {
    return mg::Weakcount(mg::AtomicCombinedRefcountIncrement(combined_refcount, mg::WEAKCOUNT_ONE));
}

// The requirement is that all modifications to the managed object happen-before
// invocation of the managed object destructor, and that allocation of the
// managed object storage happens-before deallocation of the storage.
//
// To get this ordering, all non-final decrements must synchronize-with the
// final decrement. So all non-final decrements have to store-release while the
// final decrement has to load-acquire, either directly or with the help of
// fences. But it's easiest just to have all decrements be acq-rel. And it turns
// out, on modern architectures and chips, it's also fastest.
inline uint64_t AtomicCombinedRefcountDecrement(
    std::atomic<uint64_t>& combined_refcount, uint64_t decrement
) {
    return combined_refcount.fetch_sub(decrement, std::memory_order_acq_rel) - decrement;
}

inline uint64_t AtomicWeakcountDecrement(std::atomic<uint64_t>& combined_refcount) {
    return mg::Weakcount(mg::AtomicCombinedRefcountDecrement(combined_refcount, mg::WEAKCOUNT_ONE));
}

class IntrusivePtrTarget {
protected:
    virtual ~IntrusivePtrTarget() = default;

    constexpr IntrusivePtrTarget() noexcept : m_combined_refcount(0) {}

    // Move constructor
    IntrusivePtrTarget(IntrusivePtrTarget&&) noexcept : IntrusivePtrTarget() {}

    // Move assignment
    IntrusivePtrTarget& operator=(IntrusivePtrTarget&&) noexcept { return *this; }

    // Copy constructor
    IntrusivePtrTarget(const IntrusivePtrTarget&) noexcept : IntrusivePtrTarget() {}

    // Copy assignment
    IntrusivePtrTarget& operator=(const IntrusivePtrTarget&) noexcept { return *this; }

#if defined(MINIGRAD_TESTS)
    uint32_t TestingRefcount() const { return Refcount(std::memory_order_relaxed); }
    uint32_t TestingWeakcount() const { return Weakcount(std::memory_order_relaxed); }
#endif

private:
    mutable std::atomic<uint64_t> m_combined_refcount;
    static_assert(sizeof(std::atomic<uint64_t>) == 8);
    static_assert(alignof(std::atomic<uint64_t>) == 8);

    template <class TargetType, class NullType> friend class IntrusivePtr;

    // Called when refcount is 0
    // There could still be weak references, so the object may not be used but the destructor is
    // yet to be called
    // If there are no weak references (class is about to be destructed), this method will not be
    // called
    virtual void ReleaseResources() {}

    uint32_t Refcount(std::memory_order order = std::memory_order_relaxed) const {
        return mg::Refcount(m_combined_refcount.load(order));
    }

    uint32_t Weakcount(std::memory_order order = std::memory_order_relaxed) const {
        return mg::Weakcount(m_combined_refcount.load(order));
    }

    // Increments can usually be memory_order_relaxed
    // But decrements need to check for zero needs release on the decrement itself and an acquire
    // fence, specifically on the branch that's about to delete--this avoids paying cost on every
    // decrement, only on the one that actually wins the race to destroy
};

template <class TargetType, class NullType = IntrusiveTargetDefaultNullType<TargetType>>
class IntrusivePtr final {
public:
    // Default constructor
    IntrusivePtr() noexcept : IntrusivePtr(NullType::Singleton(), mg::DontIncreaseRefCount{}) {}

    // nullptr constructor
    IntrusivePtr(std::nullptr_t) noexcept 
        : IntrusivePtr(NullType::Singleton(), mg::DontIncreaseRefCount{}) {}

    // Special case
    IntrusivePtr(TargetType* target, mg::DontIncreaseRefCount) : m_target(target) {}

    // Move constructor
    IntrusivePtr(IntrusivePtr&& rhs) noexcept : m_target(rhs.m_target) {
        rhs.m_target = NullType::Singleton();
    }

    // Converting move constructor
    // Note that the current target type must have a constructor from rhs's target type
    template <class From, class FromNullType>
    IntrusivePtr(IntrusivePtr<From, FromNullType>&& rhs) noexcept 
        : m_target(mg::AssignPtr<TargetType, NullType, FromNullType>(rhs.m_target))
    {
        static_assert(
            std::is_convertible_v<From*, TargetType*>,
            "Type mismatch: IntrusivePtr converting move constructor got pointer of invalid type.");
        rhs.m_target = FromNullType::Singleton();
    }

    // Copy constructor; increment the strong reference count
    IntrusivePtr(const IntrusivePtr& rhs) noexcept : m_target(rhs.m_target) { Retain(); }

    // Converting copy constructor
    // Note that the current target type must have a constructor from rhs's target type
    template <class From, class FromNullType>
    IntrusivePtr(const IntrusivePtr<From, FromNullType>& rhs) noexcept 
        : m_target(mg::AssignPtr<TargetType, NullType, FromNullType>(rhs.m_target))
    {
        static_assert(
            std::is_convertible_v<From*, TargetType*>,
            "Type mismatch: IntrusivePtr converting copy constructor got pointer of invalid type.");
        Retain();
    }

    // Destructor
    ~IntrusivePtr() noexcept { _Reset(); }

    // Copy assignment operator for same-type
    IntrusivePtr& operator=(const IntrusivePtr& rhs) & noexcept {
        return this->template operator=<TargetType, NullType>(rhs);
    }

    // Copy assignment
    template <class From, class FromNullType>
    IntrusivePtr& operator=(const IntrusivePtr<From, FromNullType>& rhs) & noexcept {
        static_assert(
            std::is_convertible_v<From*, TargetType*>,
            "Type mismatch: IntrusivePtr copy assignment got pointer of invalid type.");
        
        // Because rhs is a const l-value reference, this invokes the copy constructor
        // Which increments the strong ref count by 1
        IntrusivePtr tmp = rhs;

        // Swap this's target with tmp's target
        // Once tmp goes out of range, the destructor for the swapped target will be called
        Swap(tmp);
        return *this;
    }

    // Move assignment for same-type
    IntrusivePtr& operator=(IntrusivePtr&& rhs) & noexcept {
        return this->template operator=<TargetType, NullType>(std::move(rhs));
    }

    // Move assignment
    template <class From, class FromNullType>
    IntrusivePtr& operator=(IntrusivePtr<From, FromNullType>&& rhs) & noexcept {
        static_assert(
            std::is_convertible_v<From*, TargetType*>,
            "Type mismatch: IntrusivePtr move assignment got pointer of invalid type.");
        IntrusivePtr tmp = std::move(rhs);
        Swap(tmp);
        return *this;
    }

    [[nodiscard]] TargetType* Get() const noexcept { return m_target; }
    TargetType& operator*() const noexcept { return *m_target; }
    TargetType* operator->() const noexcept { return m_target; }
    operator bool() const noexcept { return m_target != NullType::Singleton(); }

    void Reset() noexcept {
        _Reset();
        m_target = NullType::Singleton();
    }

    void Swap(IntrusivePtr &rhs) noexcept { std::swap(m_target, rhs.m_target); }

    // We do a lot of null-pointer checks in our code, good to have this be cheap
    [[nodiscard]] bool Defined() const noexcept { return m_target != NullType::Singleton(); }

    [[nodiscard]] uint32_t UseCount() const noexcept {
        if (m_target == NullType::Singleton()) return 0;
        return m_target->Refcount(std::memory_order_relaxed);
    }
    
    [[nodiscard]] uint32_t WeakUseCount() const noexcept {
        if (m_target == NullType::Singleton()) return 0;
        return m_target->Weakcount(std::memory_order_relaxed);
    }

    [[nodiscard]] bool Unique() const noexcept { return UseCount() == 1; }

    template <class... Args>
    static IntrusivePtr Make(Args&&... args) {
        return IntrusivePtr(new TargetType(std::forward<Args>(args)...));
    }

private:
    static_assert(
        std::is_base_of_v<
        TargetType,
        std::remove_pointer_t<decltype(NullType::Singleton())>>,
        "NullType::Singleton() must reutrn a TargetType* pointer");

    template <class, class> friend class IntrusivePtr;

    TargetType* m_target;

    // raw pointer constructors are not public because we shouldn't make
    // intrusive_ptr out of raw pointers except from inside the make_intrusive(),
    // reclaim() and weak_intrusive_ptr::lock() implementations.

    // This constructor will increase the ref counter for you.
    // This constructor will be used by the make_intrusive(), and also pybind11,
    // which wrap the intrusive_ptr holder around the raw pointer and incref
    // correspondingly (pybind11 requires raw pointer constructor to incref by
    // default).
    explicit IntrusivePtr(TargetType* target) noexcept 
        : IntrusivePtr(target, mg::DontIncreaseRefCount{}) 
    {
        if (m_target != NullType::Singleton())
            m_target->m_combined_refcount.store(mg::UNIQUE_REF, std::memory_order_relaxed);
    }

    void Retain() noexcept {
        if (m_target != NullType::Singleton())
            mg::AtomicCombinedRefcountIncrement(m_target->m_combined_refcount, mg::REFCOUNT_ONE);
    }

    void _Reset() noexcept {
        if (m_target != NullType::Singleton()) ResetNotNull(m_target);
    }

    static void ResetNotNull(TargetType* target) noexcept {
        if (mg::IsUniquelyOwned(target->m_combined_refcount.load(std::memory_order_acquire))) {
            // Both counts are 1, so there are no weak references, hence we are releasing the
            // last strong reference. No other threads can observe the effects of this target
            // deletion call (e.g. calling UseCount()) without a data race
            target->m_combined_refcount.store(0, std::memory_order_relaxed);
            delete target;
            return;
        }

        auto combined_refcount = mg::AtomicCombinedRefcountDecrement(
            target->m_combined_refcount, REFCOUNT_ONE);
        uint32_t new_refcount = mg::Refcount(combined_refcount);

        // If the strong reference count is 0
        if (new_refcount == 0) {
            if (mg::Weakcount(combined_refcount) == 1) {
                delete target;
                return;
            }

            ReleaseResourcesAndDecrementWeakcount(target);
        }
    }

    static void ReleaseResourcesAndDecrementWeakcount(TargetType* target) noexcept {
        const_cast<std::remove_const_t<TargetType>*>(target)->ReleaseResources();
        if (mg::AtomicWeakcountDecrement(target->m_combined_refcount) == 0) delete target;
    }
};

template <
    class TargetType, 
    class NullType = IntrusiveTargetDefaultNullType<TargetType>, 
    class... Args>
[[nodiscard]] inline IntrusivePtr<TargetType, NullType> MakeIntrusive(Args&&... args) {
    return IntrusivePtr<TargetType, NullType>::Make(std::forward<Args>(args)...);
}

template <class TargetType1, class NullType1, class TargetType2, class NullType2>
[[nodiscard]] bool operator==(
    const IntrusivePtr<TargetType1, NullType1>& lhs,
    const IntrusivePtr<TargetType2, NullType2>& rhs) noexcept 
{
    return lhs.Get() == rhs.Get();
}

template <class TargetType, class NullType>
[[nodiscard]] bool operator==(
    const IntrusivePtr<TargetType, NullType>& lhs, std::nullptr_t) noexcept 
{
    return lhs.Get() == nullptr;
}

template <class TargetType1, class NullType1, class TargetType2, class NullType2>
[[nodiscard]] bool operator!=(
    const IntrusivePtr<TargetType1, NullType1>& lhs,
    const IntrusivePtr<TargetType2, NullType2>& rhs) noexcept 
{
    return !(lhs.Get() == rhs.Get());
}

template <class TargetType, class NullType>
[[nodiscard]] bool operator!=(
    const IntrusivePtr<TargetType, NullType>& lhs, std::nullptr_t) noexcept 
{
    return !(lhs.Get() == nullptr);
}

template <class TargetType, class NullType = IntrusiveTargetDefaultNullType<TargetType>>
class WeakIntrusivePtr final {
public:
    // TODO
    // Add friend class to both IntrusivePtr and WeakIntrusivePtr

private:
    TargetType* m_target;

    static_assert(
        std::is_base_of_v<
        TargetType,
        std::remove_pointer_t<decltype(NullType::Singleton())>>,
        "NullType::Singleton() must reutrn a TargetType* pointer");
};

}

namespace std {

template <class TargetType, class NullType>
struct hash<mg::IntrusivePtr<TargetType, NullType>> {
    size_t operator()(const mg::IntrusivePtr<TargetType, NullType>& ptr) const {
        return std::hash<TargetType*>()(ptr.Get());
    }
};

}