#pragma once

#include "IntrusivePtr.hpp"
#include "Node.hpp"
#include "Tensor.hpp"

namespace mg {

IntrusivePtr<Node> GradAccumulator(const Tensor& self);

}