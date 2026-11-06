#pragma once

#include "Node.hpp"
#include "IntrusivePtr.hpp"
#include "Tensor.hpp"

#include <cstdint>

namespace mg {

struct Edge {
    Edge() noexcept : m_function(nullptr), m_input_number(0) {}
    Edge(IntrusivePtr<Node> function, uint32_t input_number) noexcept
        : m_function(function)
        , m_input_number(input_number) {}

    bool IsValid() const noexcept { return m_function != nullptr; }

    bool operator==(const Edge& other) {
        return m_function == other.m_function && m_input_number == other.m_input_number;
    }

    bool operator!=(const Edge& other) {
        return !(*this == other);
    }

    IntrusivePtr<Node> m_function;
    uint32_t m_input_number;
};

Edge GradEdge(const Tensor& self);

}