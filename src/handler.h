//
// Created by Colin MacKenzie on 2019-01-16.
//

#ifndef RESTFULLY_FUNCTION_TRAITS_H
#define RESTFULLY_FUNCTION_TRAITS_H

#include <functional>
#include <memory>
#include <tuple>


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
        typedef std::tuple< TArgs... > arguments;

#if 0
        Handler() {}
        //Handler(const F0& _f) : handler(_f) {}
        Handler(F0&& _f) : handler(std::move(_f)) {}
        //Handler(std::function<TArgs...> _f) : handler(_f) {}
        Handler(int (_handler)(TArgs... args)) { handler = _handler; }
#endif
        template<class ... MArgs>
        Handler(MArgs... args) : handler(args...) {}

        int operator()(TArgs... args) {
            return handler(args...);
        }

        inline bool operator==(std::nullptr_t v) const { return handler==v; }
        inline bool operator!=(std::nullptr_t v) const { return handler!=v; }

        F0 handler;
    };

#if 0
    /*** Function Traits
     *  adapted from boost::function_traits
     */
    template<class F>
    struct function_traits;

    // function pointer
    template<class R, class... Args>
    struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {
    };

    // Remove the first item in a tuple
    template<typename T>
    struct tuple_tail;

    template<typename Head, typename ... Tail>
    struct tuple_tail<std::tuple<Head, Tail ...>>
    {
        using type = std::tuple<Tail ...>;
    };

    // Free functions
    template<class R, class... Args>
    struct function_traits<R(Args...)> {
        using return_type = R;

        using arguments = std::tuple<Args ...>;

        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        struct argument {
            static_assert(N < arity, "error: invalid parameter index.");
            using type = typename std::tuple_element<N, std::tuple < Args...>>::type;
        };

        typedef Rest::Handler<Args...> HandlerType;
        typedef std::function<return_type(Args...)> FunctionType;
    };


    // std::function
    template<typename FunctionT>
    struct function_traits
    {
        using arguments = typename tuple_tail<
                typename function_traits<decltype( & FunctionT::operator())>::arguments>::type;

        static constexpr std::size_t arity = std::tuple_size<arguments>::value;

        template<std::size_t N>
        using argument_type = typename std::tuple_element<N, arguments>::type;

        using return_type = typename function_traits<decltype( & FunctionT::operator())>::return_type;

        using HandlerType = typename function_traits<decltype( & FunctionT::operator())>::HandlerType ;
        using FunctionType = typename function_traits<decltype( & FunctionT::operator())>::FunctionType ;
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

    template<typename FunctionT>
    struct function_traits<FunctionT&>: public function_traits<FunctionT> {};

    template<typename FunctionT>
    struct function_traits<FunctionT&&>: public function_traits<FunctionT> {};

#if 0
    // std::bind for free functions
    template<typename ReturnTypeT, typename ... Args, typename ... FArgs>
#if defined _LIBCPP_VERSION  // libc++ (Clang)
    struct function_traits<std::__1::__bind<ReturnTypeT( &)(Args ...), FArgs ...>>
#elif defined __GLIBCXX__  // glibc++ (GNU C++)
    struct function_traits<std::_Bind<ReturnTypeT(*(FArgs ...))(Args ...)>>
#elif defined _MSC_VER  // MS Visual Studio
        struct function_traits<std::_Binder<std::_Unforced, ReturnTypeT(__cdecl &)(Args ...), FArgs ...>>
 #else
 #error "Unsupported C++ compiler / standard library"
#endif
            : function_traits<ReturnTypeT(Args ...)>
    {};
#endif
#else

    // Remove the first item in a tuple
    template<typename T>
    struct tuple_tail;

    template<typename Head, typename ... Tail>
    struct tuple_tail<std::tuple<Head, Tail ...>>
    {
        using type = std::tuple<Tail ...>;
    };

    // std::function
    template<typename FunctionT>
    struct function_traits
    {
        using arguments = typename tuple_tail<typename function_traits<decltype( & FunctionT::operator() )>::arguments>::type;

        static constexpr std::size_t arity = std::tuple_size<arguments>::value;

        template<std::size_t N>
        using argument_type = typename std::tuple_element<N, arguments>::type;

        using return_type = typename function_traits<decltype( & FunctionT::operator())>::return_type;

        using HandlerType = typename function_traits<decltype( & FunctionT::operator())>::HandlerType ;
        using FunctionType = typename function_traits<decltype( & FunctionT::operator())>::FunctionType ;

        //using HandlerType = typename function_traits<decltype( & FunctionT::operator())>::HandlerType ;
        //using FunctionType = typename function_traits<decltype( & FunctionT::operator())>::FunctionType ;

        //typedef Rest::Handler< arguments... > HandlerType;
        //typedef std::function<return_type( arguments... )> FunctionType;
    };

    // Free functions
    template<typename ReturnTypeT, typename ... Args>
    struct function_traits<ReturnTypeT(Args ...)>
    {
        using arguments = std::tuple<Args ...>;

        static constexpr std::size_t arity = std::tuple_size<arguments>::value;

        template<std::size_t N>
        using argument_type = typename std::tuple_element<N, arguments>::type;

        using return_type = ReturnTypeT;

        typedef Rest::Handler<Args...> HandlerType;
        typedef std::function<return_type(Args...)> FunctionType;

    };

    // Function pointers
    template<typename ReturnTypeT, typename ... Args>
    struct function_traits<ReturnTypeT (*)(Args ...)>: function_traits<ReturnTypeT(Args ...)>
    {};

    // std::bind for object methods
    template<typename ClassT, typename ReturnTypeT, typename ... Args, typename ... FArgs>
#if defined _LIBCPP_VERSION  // libc++ (Clang)
    struct function_traits<std::__1::__bind<ReturnTypeT (ClassT::*)(Args ...), FArgs ...>>
#elif defined _GLIBCXX_RELEASE  // glibc++ (GNU C++ >= 7.1)
        struct function_traits<std::_Bind<ReturnTypeT(ClassT::*(ClassT *, FArgs ...))(Args ...)>>
#elif defined __GLIBCXX__  // glibc++ (GNU C++)
struct function_traits<std::_Bind<std::_Mem_fn<ReturnTypeT (ClassT::*)(Args ...)>(FArgs ...)>>
#elif defined _MSC_VER  // MS Visual Studio
struct function_traits<
std::_Binder<std::_Unforced, ReturnTypeT(__cdecl ClassT::*)(Args ...), FArgs ...>
>
#else
#error "Unsupported C++ compiler / standard library"
#endif
            : function_traits<ReturnTypeT(Args ...)>
    {};

    // std::bind for free functions
    template<typename ReturnTypeT, typename ... Args, typename ... FArgs>
#if defined _LIBCPP_VERSION  // libc++ (Clang)
    struct function_traits<std::__1::__bind<ReturnTypeT( &)(Args ...), FArgs ...>>
#elif defined __GLIBCXX__  // glibc++ (GNU C++)
        struct function_traits<std::_Bind<ReturnTypeT(*(FArgs ...))(Args ...)>>
#elif defined _MSC_VER  // MS Visual Studio
struct function_traits<std::_Binder<std::_Unforced, ReturnTypeT(__cdecl &)(Args ...), FArgs ...>>
#else
#error "Unsupported C++ compiler / standard library"
#endif
            : function_traits<ReturnTypeT(Args ...)>
    {};

    // Lambdas
    // we must _hide_ the Class type when we recursively pass to function_traits<>
    // warning: this will also catch 'const member function pointers', but will not forward the class type
    template<typename C, typename R, typename ... Args>
    struct function_traits<R (C::*)(Args ...) const>
            : public function_traits<R(Args ...)>
    {};

    // member function pointer
    // we must _include_ the Class type when we recursively pass to function_traits<>
    template<class C, class R, class... Args>
    struct function_traits<R(C::*)(Args...)> : public function_traits<R(C *, Args...)> {
    };

    // const member function pointer
    //template<class C, class R, class... Args>
    //struct function_traits<R(C::*)(Args...) const> : public function_traits<R(C *, Args...)> {
    //};

    template<typename FunctionT>
    struct function_traits<FunctionT &>: public function_traits<FunctionT>
    {};

    template<typename FunctionT>
    struct function_traits<FunctionT &&>: public function_traits<FunctionT>
    {};

    /* NOTE(esteve):
     * VS2015 does not support expression SFINAE, so we're using this template to evaluate
     * the arity of a function.
     */
    template<std::size_t Arity, typename FunctorT>
    struct arity_comparator : std::integral_constant<
            bool, (Arity == function_traits<FunctorT>::arity)>{};

    template<typename FunctorT, typename ... Args>
    struct check_arguments : std::is_same<
            typename function_traits<FunctorT>::arguments,
            std::tuple<Args ...>
    >
    {};

    template<typename FunctorAT, typename FunctorBT>
    struct same_arguments : std::is_same<
            typename function_traits<FunctorAT>::arguments,
            typename function_traits<FunctorBT>::arguments
    >
    {};

#endif
} // ns: Rest

#endif //RESTFULLY_FUNCTION_TRAITS_H
