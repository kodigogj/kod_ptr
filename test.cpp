#include "kod_ptr.h"
#include <iostream>
#include <vector>

using namespace kod;

struct Node : kod_ptr_base {
    data* const mdata;
    counter* const mcount{ new counter{ 0 } };

    kod_ptr<Node, true> parent;
    std::vector<kod_ptr<Node>> children;

    inline Node()
        : mdata{ new default_data<Node, false>{ this } }
    {
    }
    inline Node(data* const& data)
        : mdata{ data }
    {
    }

    inline void addChild(kod_ptr<Node>& child)
    {
        child->parent = this;
        children.push_back(child);
    }

    inline data* const& get_data() const override { return mdata; }
    inline counter* const& get_counter() const override { return mcount; };
    ~Node() { std::cout << "~Node\n"; }
};

int main()
{
    {
        kod_ptr<Node> a;
        {
            kod_ptr<Node> p;
            {
                kod_ptr<Node> parent = new Node;
                p = parent;
                kod_ptr<Node> child = new Node;
                a = child;
                kod_ptr<Node> child1 = new Node;
                kod_ptr<Node> child2 = new Node;
                parent->addChild(child);
                parent->addChild(child1);
                parent->addChild(child2);
            }
            std::cin.get();
        }
        std::cin.get();
    }
    std::cin.get();
}