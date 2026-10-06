#include "IntrusivePtr.hpp"
#include <iostream>
#include <string>
#include <vector>

class Node : public mg::IntrusivePtrTarget {
public:
    using InputType = mg::IntrusivePtr<Node>;

    explicit Node(const std::string& name) noexcept
        : m_name(std::move(name))
    {
        std::cout << "Created node: " << m_name << std::endl;
    }

    virtual ~Node() { std::cout << "Destroyed node: " << m_name << std::endl; }

    const std::string& Name() const { return m_name; }
    const std::vector<InputType>& Inputs() const { return m_inputs; }

    virtual void Backward() = 0;
        
protected:
    std::string m_name;
    std::vector<InputType> m_inputs;
};

class Variable final : public Node {
public:
    Variable(const std::string& name, float val) noexcept
        : Node(std::move(name)), m_val(val) {}
    
    float Val() const { return m_val; }

    void Backward() override { std::cout << "Backward through: " << Name() << std::endl; }

private:
    float m_val;
};

class Multiply final : public Node {
public:
    Multiply(mg::IntrusivePtr<Node> node1, mg::IntrusivePtr<Node> node2) noexcept
        : Node("Multiply") 
    {
        m_inputs.push_back(std::move(node1));
        m_inputs.push_back(std::move(node2));
    }

    void Backward() override {
        std::cout << "Backward through: " << Name() << std::endl;
        for (auto& input : m_inputs) { input->Backward(); }
    }
};

class Add final : public Node {
public:
    Add(mg::IntrusivePtr<Node> node1, mg::IntrusivePtr<Node> node2) noexcept
        : Node("Add")
    {
        m_inputs.push_back(std::move(node1));
        m_inputs.push_back(std::move(node2));
    }

    void Backward() override {
        std::cout << "Backward through: " << Name() << std::endl;
        for (auto& input : m_inputs) { input->Backward(); }
    }
};

int main() {
    auto x = mg::MakeIntrusive<Variable>("x", 2.0f);
    auto y = mg::MakeIntrusive<Variable>("y", 3.0f);

    // x and y are now referenced by:
    // x -> Variable
    // y -> Variable
    // z -> Multiply -> x
    // z -> Multiply -> y
    auto z = mg::MakeIntrusive<Multiply>(x, y);

    std::cout << "x refcount: " << x.UseCount() << std::endl;
    std::cout << "y refcount: " << y.UseCount() << std::endl;
    std::cout << "z refcount: " << z.UseCount() << std::endl;

    // The user-facing references can disappear while the graph continues to own its inputs
    x.Reset();
    y.Reset();

    // Notice how the Nodes are still alive
    std::cout << "z refcount: " << z.UseCount() << std::endl;
    std::cout << "z input 1 refcount: " << (z->Inputs())[0].UseCount() << std::endl;
    std::cout << "z input 2 refcount: " << (z->Inputs())[1].UseCount() << std::endl;

    std::cout << "Backward pass: " << std::endl;
    z->Backward();
    z.Reset();
    std::cout << "Graph destroyed" << std::endl;
}