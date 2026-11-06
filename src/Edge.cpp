#include "Edge.hpp"
#include "AutogradFunctions.hpp"

namespace mg {

Edge GradEdge(const Tensor& self) {
    // If the current tensor has a grad function (say MulBackward), set the edge's grad fn to be
    // that. If it does not, set it to be AccumulateGrad.

    if (const auto& grad_fn = self.GradFn()) {
        return Edge(grad_fn, self.OutputNumber());
    } else {
        return Edge(GradAccumulator(self), 0);
    }
}

}