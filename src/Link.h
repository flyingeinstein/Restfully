//
// Created by colin on 11/9/2018.
//

#pragma once


namespace Rest {

    /// \brief defines a subclass of a given class with an added Node member for linking within an expression tree
    /// This new class acts exactly like class T, but has an added 'nextNode' member of type TNode.
    template<class T, class TNode>
    class Link : public T {
    public:
        inline Link() : nextNode(nullptr) {}

        inline Link(const T &emplace) : T(emplace), nextNode(nullptr) {}

        inline Link(const Link &copy) : T(copy), nextNode(copy.nextNode) {}

        Link &operator=(const Link &copy) {
            T::operator=(copy);
            nextNode = copy.nextNode;
            return *this;
        }

        // constructor with the same args as base
        template<class... Targs>
        explicit Link(Targs... args) : T(args...), nextNode(nullptr) {}

        TNode *nextNode;
    };
}
