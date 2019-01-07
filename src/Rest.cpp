//
// Created by ineoquest on 2/2/18.
//

#include "Rest.h"


#include <string.h>
//#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if 0   // I think this is old and was always unused
typedef enum {
    nop = 0x1000, // lets store opcodes in the upper bit so we can do some assert testing during debugging
    pushString,
    scan,
    pop,
    jmp,
    isType,
    isNotType,
    matchSep,
    matchEof,
    matchWord,
    writeParam,     // [name:stringid, :0]      write a parameter to output json
    setName,        // set the rest method name
    callEndpoint,     // set the handler ID for the rest method
    success,
    fail
} url_opcode_e;
#endif

namespace Rest {

    const Argument Argument::null;

const char* uri_method_to_string(HttpMethod method) {
    switch(method) {
        case HttpGet: return "GET";
        case HttpPost: return "POST";
        case HttpPut: return "PUT";
        case HttpPatch: return "PATCH";
        case HttpDelete: return "DELETE";
        case HttpOptions: return "OPTIONS";
        case HttpMethodAny: return "ANY";
        default: return "GET";
    }
}

const char* uri_result_to_string(short result) {
    switch(result) {
        case URL_MATCHED: return "matched";
        case URL_FAIL_NO_ENDPOINT: return "no matching endpoint";
        case URL_FAIL_NO_HANDLER: return "endpoint doesnt support requests for given http verb";
        case URL_FAIL_DUPLICATE: return "endpoint already exists";
        case URL_FAIL_PARAMETER_TYPE: return "parameter type mismatch";
        case URL_FAIL_MISSING_PARAMETER: return "missing expected parameter";
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





#if 0
short rest_uri_token_is_type(token t, uint32_t typemask)
{
    if(t.id==TID_INTEGER && (typemask & ARG_MASK_INTEGER)>0)
        return 1;
    else if(t.id==TID_FLOAT && (typemask & ARG_MASK_REAL)>0)
        return 1;
    else if(t.id==TID_BOOL && (typemask & ARG_MASK_BOOLEAN)>0)
        return 1;
    else if((t.id==TID_STRING || t.id==TID_IDENTIFIER) && (typemask & ARG_MASK_STRING)>0)
        return 1;
    else
        return 0;
}

json_object* json_object_from_token(token t)
{
    if(t.id==TID_INTEGER)
        return json_object_new_int64(t.i);
    if(t.id==TID_FLOAT)
        return json_object_new_double(t.d);
    if(t.id==TID_BOOL)
        return json_object_new_boolean(t.i>0);
    if(t.id==TID_STRING || t.id==TID_IDENTIFIER)
        return json_object_new_string(t.s);
    return nullptr;
}
#endif



#if 0
yarn* rest_uri_debug_print_internal(UriExpression* expr, ParseData* ev, yarn* out, int level)
{
    Endpoint* ep = ev->ep;
    Argument* arg;
    {
        // print what method handlers are attached to this endpoint, if any
        if(ep->GET!=nullptr | ep->POST!=nullptr | ep->PUT!=nullptr | ep->PATCH!=nullptr | ep->DELETE!=nullptr | ep->OPTIONS!=nullptr) {
            yarn_printf(out, "[");
            yarn_join(out, ',', (const char* []){
                    ep->GET?"GET":nullptr,
                    ep->POST?"POST":nullptr,
                    ep->PUT?"PUT":nullptr,
                    ep->PATCH?"PATCH":nullptr,
                    ep->DELETE?"DELETE":nullptr,
                    ep->OPTIONS?"OPTIONS":nullptr
            }, 6);
            yarn_printf(out, "]");
        }

        // loop through all literals that are acceptable from this endpoint
        Literal* lit = ep->literals;
        if(lit) {
            // if we have more than 1 literal, then put on a second line
            BOOL multiple = rest_ep_literal_count(lit) >1;

            while (rest_ep_literal_isValid(lit)) {
                if(multiple)
                    yarn_printf(out, "\n%*s ·", level*2, "");

                if (lit->isNumeric)
                    yarn_printf(out, "/%d", lit->id);
                else
                    yarn_printf(out, "/%s", binbag_get(expr->text, lit->id));

                // recurse if we have more endpoints to travel
                if (lit->next) {
                    ev->ep = lit->next;
                    rest_uri_debug_print_internal(expr, ev, out, level+1);
                    ev->ep = ep;
                }
                lit++;
            }
        }

        // figure out what types are grouped together
        // determine the typemask of any already set handlers at this endpoint.
        // we cannot have two different handlers that handle the same type, but if the typemask
        // exactly matches we can just consider a match and jump to that endpoint.
        uint16_t tm_values[3] = { ARG_MASK_NUMBER, ARG_MASK_STRING, ARG_MASK_BOOLEAN };
        Argument* tm_handlers[3] = { ep->numeric, ep->string, ep->boolean };

        // we loop through our list of handlers, we save the first non-nullptr handler encountered and
        // then find more instances of that handler and build a typemask. We set the handlers in our
        // list of handlers to nullptr for each matching one so eventually we will have all nullptr handlers
        // and each distinct handler will have been checked.
        arg = nullptr;
        while(arg==nullptr && (tm_handlers[0]!=nullptr || tm_handlers[1]!=nullptr || tm_handlers[2]!=nullptr)) {
            int i;
            uint16_t tm=0;
            Argument *x=nullptr, *y=nullptr;
            for(i=0; i<sizeof(tm_handlers)/sizeof(tm_handlers[0]); i++) {
                if(tm_handlers[i]!=nullptr) {
                    if(x==nullptr) {
                        // captured the first non-nullptr element
                        x = tm_handlers[i];
                        tm |= tm_values[i];
                        tm_handlers[i]=0;
                    } else if(x==tm_handlers[i]){
                        // already encountered the first element, found a second match
                        tm |= tm_values[i];
                        tm_handlers[i]=0;
                    }
                }
            }

            if(x!=nullptr) {
                size_t i;
                char s[128];
                char* types[3] = {0};

                assert(tm>0); // must have gotten at least some typemask then
                if((tm & ARG_MASK_NUMBER) == ARG_MASK_INTEGER) {
                    sprintf(s, "%s:int", ep->numeric->name);
                    types[0] = strdup(s);
                }
                else if((tm & ARG_MASK_NUMBER) == ARG_MASK_REAL) {
                    sprintf(s, "%s:real", ep->numeric->name);
                    types[0] = strdup(s);
                }
                else if((tm & ARG_MASK_NUMBER) == ARG_MASK_NUMBER) {
                    sprintf(s, "%s:number", ep->numeric->name);
                    types[0] = strdup(s);
                }
                if((tm & ARG_MASK_STRING) == ARG_MASK_STRING) {
                    sprintf(s, "%s:string", ep->string->name);
                    types[1] = strdup(s);
                }
                if((tm & ARG_MASK_BOOLEAN) == ARG_MASK_BOOLEAN) {
                    sprintf(s, "%s:boolean", ep->boolean->name);
                    types[2] = strdup(s);
                }

                yarn_printf(out, "\n%*s ·/<", level*2, "");
                yarn_join(out, '|', types, sizeof(types)/sizeof(types[0]));
                yarn_printf(out, ">");

                for(i=0; i<sizeof(types)/sizeof(types[0]); i++)
                    if(types[i]!=nullptr)
                        free(types[i]);

                // recurse if we have more endpoints to travel
                if (x->next) {
                    ev->ep = x->next;
                    rest_uri_debug_print_internal(expr, ev, out, level+1);
                    ev->ep = ep;
                }

            }
        }

    }
    return out;
}

yarn* rest_uri_debug_print(UriExpression* expr, yarn* out)
{
    ParseData ev;
    if(rest_uri_init_eval_data(expr, &ev, nullptr)!=0)
        return nullptr;
    if(out==nullptr)
        out = yarn_create(1000);
    return rest_uri_debug_print_internal(expr, &ev, out, 0);
}
#endif


} // ns: Rest
