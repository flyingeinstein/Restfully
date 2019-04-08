//
// Created by colin on 11/9/2018.
//

#pragma once

namespace Rest {

    class Literal {
    public:
        Literal() : id(0), isNumeric(false), nextLiteral(nullptr) {}
        Literal(ssize_t _id, bool _isNumeric = false) : id(_id), isNumeric(_isNumeric), nextLiteral(nullptr) {}

        Literal(const Literal& copy) : id(copy.id), isNumeric(copy.isNumeric), nextLiteral(copy.nextLiteral) {}

        // if this argument is matched, the value is added to the request object under this field name
        // this id usually indicates an index into an array of text terms (binbag)
        ssize_t id;

        // true if the id should be take as a numeric value and not a string index ID
        bool isNumeric;

        // pointer to the next literal in the linked list of literals
        Literal* nextLiteral;

        inline bool isValid() { return isNumeric || (id >= 0); }
    };

} // ns: Rest