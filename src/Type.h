//
// Created by guru on 9/11/20.
//

#ifndef RESTFULLY_TYPE_H
#define RESTFULLY_TYPE_H


// Bitmask values for different types
// todo:  convert this to an enum, combine with Token type, and add typeid info for Object types
#define ARG_MASK_INTEGER       ((unsigned short)1)
#define ARG_MASK_REAL          ((unsigned short)2)
#define ARG_MASK_BOOLEAN       ((unsigned short)4)
#define ARG_MASK_STRING        ((unsigned short)8)
#define ARG_MASK_UNSIGNED      ((unsigned short)16)
#define ARG_MASK_OBJECT        ((unsigned short)64)
#define ARG_MASK_CONST_OBJECT  ((unsigned short)128)
#define ARG_MASK_UINTEGER      (ARG_MASK_UNSIGNED|ARG_MASK_INTEGER)
#define ARG_MASK_NUMBER        (ARG_MASK_REAL|ARG_MASK_INTEGER)
#define ARG_MASK_ANY           (ARG_MASK_NUMBER|ARG_MASK_BOOLEAN|ARG_MASK_STRING)


namespace Rest {

    class Type
    {
    protected:
        // collection of ARG_MASK_xxxx bits represent what types are supported for this argument
        unsigned short type_mask;

        inline void setType(unsigned short m) { type_mask = m; }

    public:
        inline Type() : type_mask(0) {}
        Type(unsigned short _typemask) : type_mask(_typemask) {}

        inline unsigned short typemask() const { return type_mask; }
        inline bool supports(unsigned short mask) const { return (mask & type_mask)==mask; }

        inline bool operator==(unsigned short _type) const { return _type == type_mask; }
        inline bool operator==(const Type& rhs) const { return type_mask == rhs.type_mask; }

        inline bool isVoid() const { return type_mask == 0; }
        inline bool isInteger() const { return (type_mask & ARG_MASK_INTEGER); }
        inline bool isSigned() const { return (type_mask & ARG_MASK_INTEGER) && (type_mask & ARG_MASK_UNSIGNED)==0; }
        inline bool isUnsigned() const { return (type_mask & ARG_MASK_INTEGER) && (type_mask & ARG_MASK_UNSIGNED); }
        inline bool isReal() const { return (type_mask & ARG_MASK_REAL); }
        inline bool isBoolean() const { return (type_mask & ARG_MASK_BOOLEAN); }
        inline bool isString() const { return (type_mask & ARG_MASK_STRING); }
        inline bool isObject() const { return (type_mask & (ARG_MASK_OBJECT | ARG_MASK_CONST_OBJECT)); }
        inline bool isConstObject() const { return (type_mask & ARG_MASK_CONST_OBJECT); }
    };

} // ns:Rest

#endif //RESTFULLY_TYPE_H
