//
// Created by guru on 9/10/20.
//
#include "Exception.h"

namespace Rest {

    const char *uri_result_to_string(short result) {
        switch (result) {
            case UriMatched: return "matched";
            case NoEndpoint: return "no matching endpoint";
            case NoHandler: return "endpoint doesnt support requests for given http verb";
            case Duplicate: return "endpoint already exists";
            case InvalidParameterType: return "parameter type mismatch";
            case MissingParameter: return "missing expected parameter";
            case URL_FAIL_AMBIGUOUS_PARAMETER: return "ambiguous parameter type in endpoint declaration";
            case URL_FAIL_EXPECTED_PATH_SEPARATOR: return "expected path separator";
            case URL_FAIL_EXPECTED_EOF: return "expected end of input";
            case URL_FAIL_INVALID_TYPE: return "invalid type";
            case URL_FAIL_SYNTAX: return "syntax error";
            case URL_FAIL_INTERNAL: return "internal error";
            case URL_FAIL_INTERNAL_BAD_STRING: return "internal error: bad string reference";
            default: return "unspecified error";
        }
    }

}