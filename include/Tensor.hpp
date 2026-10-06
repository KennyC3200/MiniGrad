#pragma once

#include "IntrusivePtr.hpp"
#include "TensorImpl.hpp"
#include <cstddef>

namespace mg {

class TensorBase {
public:
    TensorBase() = default;
    explicit TensorBase(IntrusivePtr<TensorImpl> impl) : m_impl(std::move(impl)) {}

protected:
    mg::IntrusivePtr<TensorImpl> m_impl;
};

class Tensor : public TensorBase {
public:
};

}