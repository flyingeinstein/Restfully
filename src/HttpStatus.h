//
// Created by guru on 9/13/20.
//

#ifndef RESTFULLY_HTTPSTATUS_H
#define RESTFULLY_HTTPSTATUS_H

namespace Rest {
    typedef enum {
        // 2xx Success
        OK = 200,
        Success = 200,
        Created = 201,
        Accepted = 202,
        NoContent = 204,
        PartialContent = 206,

        // 4xx Client Error
        BadRequest = 400,
        Unauthorized = 401,
        Forbidden = 403,
        NotFound = 404,
        MethodNotAllowed = 405,
        NotAcceptable = 406,
        RequestTimeout = 408,

        // 5xx Server Error
        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnavailable = 503,
        GatewayTimeout = 504,
        InsufficientStorage = 507,
        NetworkAuthenticationRequired = 510,
        NetworkReadTimeoutError = 598,
        NetworkConnectTimeoutError = 599
    } HttpStatus;
}

#endif //RESTFULLY_HTTPSTATUS_H
