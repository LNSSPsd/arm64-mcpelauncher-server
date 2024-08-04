#pragma once

namespace shim {

    namespace detail {

        template <bool Cond, typename IfTrue, typename IfFalse>
        struct type_resolver;

        template <typename IfTrue, typename IfFalse>
        struct type_resolver<true, IfTrue, IfFalse> {
            using type = IfTrue;
        };

        template <typename IfTrue, typename IfFalse>
        struct type_resolver<false, IfTrue, IfFalse> {
            using type = IfFalse;
        };

        template <typename Host, typename Wrapper>
        struct wrapper_type_resolver {
            static constexpr bool is_wrapped = sizeof(Host) > sizeof(Wrapper);
            using host_type = Host;
            using wrapper_type = Wrapper;
            using type = typename type_resolver<is_wrapped, Wrapper, Host>::type;
        };


        template <typename T>
        struct unwrap_pointer {
            using type = T;
        };
        template <typename T>
        struct unwrap_pointer<T*> {
            using type = T;
        };


        template <typename Wrapper, typename... Args>
        int make_c_wrapped(Wrapper *object, int (*constructor)(decltype(object->wrapped), Args...), Args... args) {
            object->wrapped = new typename unwrap_pointer<decltype(object->wrapped)>::type;
            int ret = constructor(object->wrapped, args...);
            if (ret)
                delete object->wrapped;
            return ret;
        }

        template <typename Wrapper>
        int destroy_c_wrapped(Wrapper *object, int (*destructor)(decltype(object->wrapped))) {
            int ret = destructor(object->wrapped);
            delete object->wrapped;
            return ret;
        }

        template <typename Resolver, typename... Args>
        int make_c_wrapped_via_resolver(typename Resolver::type *object, int (*constructor)(typename Resolver::host_type *, Args...), Args... args) {
            if constexpr (Resolver::is_wrapped)
                return make_c_wrapped<typename Resolver::type, Args...>(object, constructor, args...);
            else
                return constructor(object, args...);
        }

        template <typename Resolver>
        int destroy_c_wrapped_via_resolver(typename Resolver::type *object, int (*destructor)(typename Resolver::host_type *)) {
            if constexpr (Resolver::is_wrapped)
                return destroy_c_wrapped<typename Resolver::type>(object, destructor);
            else
                return destructor(object);
        }

    }

}