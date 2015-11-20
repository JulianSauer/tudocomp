#ifndef TUDOCOMP_REGISTRY_H
#define TUDOCOMP_REGISTRY_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <istream>
#include <streambuf>
#include <sstream>
#include <string>
#include <map>
#include <functional>
#include <unordered_map>
#include <tuple>
#include <type_traits>

#include "boost/utility/string_ref.hpp"
#include "glog/logging.h"

#include "rule.h"
#include "rules.h"
#include "tudocomp_env.h"

namespace tudocomp_driver {

using namespace tudocomp;

// implementation details, users never invoke these directly
namespace tuple_call
{
    template <typename F, typename Tuple, bool Done, int Total, int... N>
    struct call_impl
    {
        static void call(F f, Tuple && t)
        {
            call_impl<F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
        }
    };

    template <typename F, typename Tuple, int Total, int... N>
    struct call_impl<F, Tuple, true, Total, N...>
    {
        static void call(F f, Tuple && t)
        {
            f(std::get<N>(std::forward<Tuple>(t))...);
        }
    };
}

// user invokes this
template <typename F, typename Tuple>
void call(F f, Tuple && t)
{
    typedef typename std::decay<Tuple>::type ttype;
    tuple_call::call_impl<F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
}

template<class Base, class T, class ... Args>
Base* construct(Args ... args) {
    return new T(args...);
}

template<class T>
struct Algorithm {
    /// Human readable name
    std::string name;
    /// Used as id string for command line and output filenames
    std::string shortname;
    /// Description text
    std::string description;
    /// Algorithm
    std::function<T*(Env&, boost::string_ref&)> algorithm;
};

template<class T>
using Registry = std::vector<Algorithm<T>>;

template<class T>
class AlgorithmRegistry;

template<class T, class SubT, class ... SubAlgos>
struct AlgorithmBuilder {
    Env& m_env;
    Algorithm<T> info;
    Registry<T>& registry;
    std::tuple<AlgorithmRegistry<SubAlgos>...> sub_algos;

    template<class U>
    AlgorithmBuilder<T, SubT, SubAlgos..., U>
    with_sub_algos(std::function<void (AlgorithmRegistry<U>&)> f);

    inline void do_register();
};

template<class T>
class AlgorithmRegistry {
    Env& m_env;
    std::string name = "?";
public:
    inline AlgorithmRegistry(Env& env): m_env(env) { }

    std::vector<Algorithm<T>> registry = {};

    template<class U>
    AlgorithmBuilder<T, U> with_info(std::string name,
                                     std::string shortname,
                                     std::string description) {
        Algorithm<T> algo {
            name,
            shortname,
            description,
            nullptr
        };
        AlgorithmBuilder<T, U> builder {
            m_env,
            algo,
            registry,
            {}
        };
        //
        return builder;
    }

    inline Algorithm<T>* findByShortname(boost::string_ref s) {
        for (auto& x: registry) {
            if (x.shortname == s) {
                return &x;
            }
        }
        return nullptr;
    }

    inline void set_name(std::string n) {
        name = n;
    }
};

template<class T, class SubT, class ... SubAlgos>
template<class U>
AlgorithmBuilder<T, SubT, SubAlgos..., U> AlgorithmBuilder<T, SubT, SubAlgos...>
        ::with_sub_algos(std::function<void(AlgorithmRegistry<U>&)> f) {
    AlgorithmRegistry<U> reg(m_env);
    f(reg);
    return {
        m_env,
        info,
        registry,
        std::tuple_cat(sub_algos, std::tuple<AlgorithmRegistry<U>>{reg})
    };
}

inline boost::string_ref pop_algorithm_id(boost::string_ref& algorithm_id) {
    auto idx = algorithm_id.find('.');
    boost::string_ref r = algorithm_id.substr(0, idx);
    algorithm_id.remove_prefix(idx);
    return r;
}

template<class T>
inline T* select_algo_or_exit(AlgorithmRegistry<T>& reg,
                              Env& env,
                              boost::string_ref& a_id) {

    auto this_id = pop_algorithm_id(a_id);
    Algorithm<T>* r = reg.findByShortname(this_id);

    if (r == nullptr) {
        // TODO
        throw std::runtime_error("unknown algo");
    }

    return r->algorithm(env, a_id);
}

template<class T, class SubT, class ... SubAlgos>
inline void AlgorithmBuilder<T, SubT, SubAlgos...>::do_register() {
    info.algorithm = [=](Env& env, boost::string_ref& a_id) -> T* {
        SubT* r;
        call(
            [=, &env, &r, &a_id](AlgorithmRegistry<SubAlgos> ... args) {
                r = new SubT(env, select_algo_or_exit(args, env, a_id)...);
            },
            sub_algos
        );
        return r;
    };
    registry.push_back(info);
};

}

#endif
