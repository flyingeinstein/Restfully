//
// Created by Colin MacKenzie on 2019-02-04.
//

#ifndef RESTFULLY_EXCEPTION_H
#define RESTFULLY_EXCEPTION_H

namespace Rest {

    template<class TNode>
    class Exception {
    public:
        TNode node;
        short code;

        inline Exception(TNode _node, short _code) : node(_node), code(_code) {}
    };

}

#endif //RESTFULLY_EXCEPTION_H
