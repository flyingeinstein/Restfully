//
// Created by Colin MacKenzie on 2019-02-04.
//

#ifndef RESTFULLY_EXCEPTION_H
#define RESTFULLY_EXCEPTION_H

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
        MissingParameter                    = -502,
        URL_FAIL_AMBIGUOUS_PARAMETER        = -503,
        URL_FAIL_EXPECTED_PATH_SEPARATOR    = -504,
        URL_FAIL_EXPECTED_EOF               = -505,
        URL_FAIL_INVALID_TYPE               = -506,
        URL_FAIL_SYNTAX                     = -507,
        URL_FAIL_INTERNAL                   = -508,
        URL_FAIL_INTERNAL_BAD_STRING        = -509,
        URL_FAIL_NULL_ROOT                  = -510,
        URL_FAIL_EXPECTED_IDENTIFIER        = -511,
        URL_FAIL_EXPECTED_STRING            = -512
    } ParseResult;

    /// \brief Convert a return value to a string.
    /// Typically use this to get a human readable string for an error result.
    const char* uri_result_to_string(short result);


    class Exception {
    public:
        short code;

        inline Exception(short _code) : code(_code) {}

        inline const char* message() const { return uri_result_to_string(code); }
    };

}

#endif //RESTFULLY_EXCEPTION_H
