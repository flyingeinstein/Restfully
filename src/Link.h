//
// Created by colin on 11/9/2018.
//

#pragma once


namespace Rest {

    /// \brief defines a subclass of a given class with an added Node member for linking within an expression tree
    /// This new class acts exactly like class T, but has an added 'next' member of type TNode.
    template<class T, class TNode>
    class Link : public T {
    public:
        inline Link() : next(nullptr) {}

        inline Link(const T &emplace) : T(emplace), next(nullptr) {}

        inline Link(const Link &copy) : T(copy), next(copy.next) {}

        Link &operator=(const Link &copy) {
            T::operator=(copy);
            next = copy.next;
            return *this;
        }

        // constructor with the same args as base
        template<class... Targs>
        explicit Link(Targs... args) : T(args...), next(nullptr) {}

        TNode *next;
    };
}
