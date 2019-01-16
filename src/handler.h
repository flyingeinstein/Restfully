//
// Created by Colin MacKenzie on 2019-01-16.
//

#ifndef RESTFULLY_FUNCTION_TRAITS_H
#define RESTFULLY_FUNCTION_TRAITS_H

namespace Rest {

    // indicates a default Rest handler that matches any http verb request
// this enum belongs with the web servers HTTP_GET, HTTP_POST, HTTP_xxx constants
    typedef enum {
        HttpMethodAny = 0,
        HttpGet,
        HttpPost,
        HttpPut,
        HttpPatch,
        HttpDelete,
        HttpOptions,
    } HttpMethod;

/// \brief Convert a http method enum value to a string.
    const char* uri_method_to_string(HttpMethod method);


    template<class... TArgs>
    class Handler {
    public:
        typedef std::function< int(TArgs... args) > F0;

        Handler() {}
        Handler(F0 _f) : method(HttpGet), handler(std::move(_f)) {}
        Handler(HttpMethod m, F0 _f) : method(m), handler(std::move(_f)) {}
        //Handler(int _f(TArgs... args)) : f0(_f) {}

        int operator()(TArgs... args) {
            return handler(args...);
        }

        HttpMethod method;
        F0 handler;
    };

    /*** Function Traits
     *  adapted from boost::function_traits
     */
    template<class F>
    struct function_traits;

    // function pointer
    template<class R, class... Args>
    struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {
    };

    template<class R, class... Args>
    struct function_traits<R(Args...)> {
        using return_type = R;

        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        struct argument {
            static_assert(N < arity, "error: invalid parameter index.");
            using type = typename std::tuple_element<N, std::tuple < Args...>>::type;
        };

        typedef Rest::Handler<Args...> HandlerType;
        typedef std::function<return_type(Args...)> FunctionType;
    };

    // member function pointer
    template<class C, class R, class... Args>
    struct function_traits<R(C::*)(Args...)> : public function_traits<R(C *, Args...)> {
    };

    // const member function pointer
    template<class C, class R, class... Args>
    struct function_traits<R(C::*)(Args...) const> : public function_traits<R(C *, Args...)> {
    };

    // member object pointer
    //template<class C, class R>
    //struct function_traits<R(C::*)> : public function_traits<R(C&)>  {};

} // ns: Rest

#endif //RESTFULLY_FUNCTION_TRAITS_H
