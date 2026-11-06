#pragma once

#include "IntrusivePtr.hpp"
#include "TensorImpl.hpp"

#include <cstdint>

namespace mg {

struct Node;

class TensorBase {
public:
    TensorBase() = default;
    explicit TensorBase(IntrusivePtr<TensorImpl> impl) : m_impl(std::move(impl)) {}

    IntrusivePtr<Node> GradFn() const;

    uint32_t OutputNumber() const;

protected:
    mg::IntrusivePtr<TensorImpl> m_impl;
};

class Tensor : public TensorBase {
public:
};

}