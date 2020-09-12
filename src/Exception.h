//
// Created by Colin MacKenzie on 2019-02-04.
//

#ifndef RESTFULLY_EXCEPTION_H
#define RESTFULLY_EXCEPTION_H

#include <exception>

namespace Rest {

    typedef enum {
        UriMatched                          = 1,
        UriMatchedWildcard                  = 2,
        UriAccepted                         = 3,
        InvalidHandler                      = -402,
        NoHandler                           = -403,
        NoEndpoint                          = -404,
        UriUnsupportedContentType           = -405,
        Duplicate                           = -406,
        InvalidParameterType                = -501,
        ConstParameterType                  = -502,
        MissingParameter                    = -503,
        URL_FAIL_AMBIGUOUS_PARAMETER        = -504,
        URL_FAIL_EXPECTED_PATH_SEPARATOR    = -505,
        URL_FAIL_EXPECTED_EOF               = -506,
        URL_FAIL_INVALID_TYPE               = -507,
        URL_FAIL_SYNTAX                     = -508,
        URL_FAIL_INTERNAL                   = -509,
        URL_FAIL_INTERNAL_BAD_STRING        = -510,
        URL_FAIL_NULL_ROOT                  = -511,
        URL_FAIL_EXPECTED_IDENTIFIER        = -512,
        URL_FAIL_EXPECTED_STRING            = -513
    } ParseResult;

    /// \brief Convert a return value to a string.
    /// Typically use this to get a human readable string for an error result.
    const char* uri_result_to_string(short result);


    class Exception : public std::exception {
    public:
        short code;

        inline Exception(short _code) : code(_code) {}

        char const* what() const noexcept override {
            return uri_result_to_string(code);
        }
    };

}

#endif //RESTFULLY_EXCEPTION_H
