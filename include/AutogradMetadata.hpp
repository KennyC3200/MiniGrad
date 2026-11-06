#pragma once

#include "Tensor.hpp"
#include "Node.hpp"

#include <cstdint>

namespace mg {

struct AutogradMetadata final {
    bool m_requires_grad = false;

    // Undefined until the first backprop writes into it
    Tensor m_grad;

    // Null for leaf tensors
    IntrusivePtr<Node> m_grad_fn;

    // Which output of m_grad_fn this tensor is, for ops with several outputs
    // By default, it is 0, by assuming 1 output only
    std::uint32_t m_output_number = 0;
};

}