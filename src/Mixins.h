//
// Created by colin on 11/9/2018.
//

#pragma once


namespace Rest {

    /// \brief A Mixin combines a base primary type with one or more other class types.
    /// This can be used to generate a class that is a combination of other classes together. In Restfully, we use this
    /// to create linked lists from a base type such as Literal, Argument or External.
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

    /// \brief Same as the Mixin class but with built-in linked list functionality
    /// Although mixing in a type that links to another class type such as we do with Node, It's messy trying to mix-in
    /// a 'next' member whose defined type is the template it is mixed into. (Just try to wrap your head around that and
    /// you see why.) So this class makes forming a linked list of a Mixin simple.
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

        /* Linked list part of Mixin
         */
        LinkedMixin* next;

        void append(LinkedMixin* element) {
            LinkedMixin* e = last();
            e->next = element;
        }

        LinkedMixin* last() {
            LinkedMixin* e = this;
            while(e->next)
                e = e->next;
            return e;
        }
    };

    /// a trival version of LinkedMixin is one without any mixins
    template<class TBase>
    using Linked = LinkedMixin< TBase >;
}
