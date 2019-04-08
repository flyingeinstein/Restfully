//
// Created by colin on 11/9/2018.
//

#pragma once


namespace Rest {

    /// \brief defines a subclass of a given class with an added Node member for linking within an expression tree
    /// This new class acts exactly like class T, but has an added 'nextNode' member of type TNode.
    ///
    /// This is currently used to wrap Literal and Argument class. It would be prefered to derive these classes from
    /// a common class containing TNode* link but since TNode is not known it would mean converting these classes to
    /// templates and thus they would just add code bloat. Downside here is occasionally we will have to static_cast<>
    /// when converting T up to Link<T>.


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

    template<class TBase, class ... Links>
    class Mixin : public TBase, public Links... {
    public:
        inline Mixin() {}

        inline Mixin(const TBase &emplace) : TBase(emplace) {}

        // constructor with the same args as base
        template<class... Targs>
        explicit Mixin(Targs... args) : TBase(args...) {}

        inline Mixin(const Mixin &copy) : TBase(copy), Links(copy)... {}

        Mixin &operator=(const Mixin &copy) {
            TBase::operator=(copy);
            pack_expand_invoke( Links::operator=(copy)... );
            return *this;
        }

    protected:
        /// a fake function that facilitates variadic template pack expansion for functions (or operators)
        /// required since pack expansion is limited to certain contexts and not one of which is calling a function for
        /// each type in the parameter pack. We can however do the same thing by invoking the function for each argument
        /// of an outer function call. pack_expand_invoke(...) is the outer function that does this. Since it is inline
        /// it just gets optimized out anyway.
        inline void pack_expand_invoke(Links ... links) {}
    };

    template<class TBase, class ... Links>
    class LinkedMixin : public Mixin<TBase, Links...> {
    public:
        LinkedMixin() : next(nullptr) {}

        LinkedMixin(const TBase& emplace) : Mixin<TBase, Links...>(emplace), next(nullptr) {}

        LinkedMixin(const LinkedMixin& copy)
            : Mixin<TBase, Links...>( (Mixin<TBase, Links...>&)copy),
              next(copy.next)
        {}

        // constructor with the same args as base
        template<class... Targs>
        explicit LinkedMixin(Targs... args) : Mixin<TBase, Links...>(args...), next(nullptr) {}

        inline LinkedMixin& operator=(const LinkedMixin& copy) {
            Mixin<TBase, Links...>::operator=(copy);
            next = copy.next;
            return *this;
        }

        LinkedMixin* next;
    };
}
