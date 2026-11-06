#pragma once

#include "IntrusivePtr.hpp"
#include "Tensor.hpp"
#include "Edge.hpp"

#include <vector>

namespace mg {

struct Node : public IntrusivePtrTarget {
    virtual std::vector<Node> Apply(std::vector<Tensor>&& grads) = 0;
    std::vector<Edge> m_edges;
};

}