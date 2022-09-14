#pragma once

#include <string>
#include <type_traits>
#include "errors.hpp"

// vector type, up to user to choose to override
#ifndef ARCHIMEDES_VECTOR_TYPE
#include <vector>
#define ARCHIMEDES_VECTOR_TYPE std::vector
#endif

// map type, up to user to choose to override
#ifndef ARCHIMEDES_MAP_TYPE
#include <unordered_map>
#define ARCHIMEDES_MAP_TYPE std::unordered_map
#endif

// set type, up to user to choose to override
#ifndef ARCHIMEDES_SET_TYPE
#include <unordered_set>
#define ARCHIMEDES_SET_TYPE std::unordered_set
#endif

namespace archimedes {
template <typename T>
using vector = ARCHIMEDES_VECTOR_TYPE<T>;

template <typename K, typename V>
using map = ARCHIMEDES_MAP_TYPE<K, V>;

template <typename T, typename Hash = std::hash<T>>
using set = ARCHIMEDES_SET_TYPE<T>;

// default getter for name_map<V>
// see name_map<V, N>
template <typename V>
    requires (
        requires (V v) {
            { v.name } -> std::convertible_to<std::string>;
        })
struct name_map_default_getter {
    std::string operator()(const V &v) const {
        return v.name;
    }
};

// name_map<V, N> is a map<std::string_view, V> which gets keys (names) from
// by using (N::operator()(const V&) const) to retrieve string_view names under
// initialization
template <typename V, typename N = name_map_default_getter<V>>
struct name_map : public map<std::string, V> {
    using Base = map<std::string, V>;
    using Base::Base;

    using key_type = typename Base::key_type;
    using mapped_type = typename Base::mapped_type;
    using hasher = typename Base::hasher;
    using key_equal = typename Base::key_equal;
    using allocator_type = typename Base::allocator_type;
    using value_type = typename Base::value_type;
    using reference = typename Base::reference;
    using const_reference = typename Base::const_reference;

    name_map(std::initializer_list<V> &&vs) {
        const auto n = N();
        for (const auto &v : vs) {
            (*this)[n(v)] = v;
        }
    }
};

template <typename T>
concept is_map = requires {
    typename T::key_type;
    typename T::value_type;
    typename T::mapped_type;
};

template <typename T>
concept is_sequence = requires (const T &t) {
    typename T::value_type;
    { t[0] } -> std::same_as<const typename T::value_type&>;
    { t.data() } -> std::same_as<const typename T::value_type*>;
};

template <typename T>
concept can_push_back = requires (T &t, typename T::value_type v) {
     { t.push_back(v) };
};

namespace detail {
template <typename It, typename T>
struct transformed_iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = void;
    using value_type        = T;
    using pointer           = const value_type*;
    using reference         = const value_type&;

    transformed_iterator() requires std::is_default_constructible_v<It>
        = default;

    template <typename F>
    transformed_iterator(It &&it, It &&end, F &&f)
        : it(std::move(it)),
          end(std::move(end)),
          f(std::forward<F>(f)) {
        if (this->it != this->end) {
            this->t = this->f(*this->it);
        }
    }

    reference operator*() const {
        if (!this->t) { ARCHIMEDES_FAIL("nullopt"); }
        return *this->t;
    }
    pointer operator->() const {
        if (!this->t) { ARCHIMEDES_FAIL("nullopt"); }
        return &*this->t;
    }

    transformed_iterator &operator++() {
        if (this->it != this->end) { ++this->it; }
        if (this->it != this->end) { this->t = this->f(*this->it); }
        return *this;
    }

    transformed_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(
        const transformed_iterator &a,
        const transformed_iterator &b) {
        return a.it == b.it;
    }

    friend bool operator!=(
        const transformed_iterator &a,
        const transformed_iterator &b) {
        return a.it != b.it;
    }

private:
    std::optional<T> t = std::nullopt;
    It it, end;
    std::function<T(const typename It::value_type&)> f;
};

template <typename C, typename F>
static inline auto make_transformed_it(const C &cs, F &&f) {
    return transformed_iterator<
        typename C::const_iterator,
        std::invoke_result_t<F, const typename C::value_type &>>(
            cs.begin(), cs.end(), std::forward<F>(f));
}

template <typename C, typename F>
static inline auto make_transformed_end(const C &cs, F &&f) {
    return transformed_iterator<
        typename C::const_iterator,
        std::invoke_result_t<F, const typename C::value_type &>>(
            cs.end(), cs.end(), std::forward<F>(f));
}

template <typename M>
    requires is_map<M>
struct map_key_iterator : public M::const_iterator {
    using Base = typename M::const_iterator;

    using iterator_category = std::forward_iterator_tag;
    using difference_type   = void;
    using value_type        = typename M::key_type;
    using pointer           = const value_type*;
    using reference         = const value_type&;

    map_key_iterator(Base &&it)
        : Base(std::move(it)) {}

    reference operator*() const {
        return Base::operator*().second;
    }

    pointer operator->() const {
        return Base::operator*().second;
    }
};

template <typename M>
    requires is_map<M>
struct map_value_iterator : public M::const_iterator {
    using Base = typename M::const_iterator;

    using iterator_category = std::forward_iterator_tag;
    using difference_type   = void;
    using value_type        = typename M::mapped_type;
    using pointer           = const value_type*;
    using reference         = const value_type&;

    map_value_iterator(Base &&it)
        : Base(std::move(it)) {}

    reference operator*() const {
        return Base::operator*().second;
    }

    pointer operator->() const {
        return Base::operator*().second;
    }
};

template <typename It>
struct iterable {
    using value_type = typename It::value_type;

    iterable(It &&begin, It &&end)
        : it_begin(begin),
          it_end(end) {}

    It begin() const { return this->it_begin; }
    It end() const { return this->it_end; }

private:
    It it_begin, it_end;
};

template <typename M>
    requires is_map<M>
inline auto keys(const M &m) {
    return iterable<map_key_iterator<M>>(
        map_key_iterator<M>(m.begin()),
        map_key_iterator<M>(m.end()));
}

template <typename M>
    requires is_map<M>
inline auto values(const M &m) {
    return iterable<map_value_iterator<M>>(
        map_value_iterator<M>(m.begin()),
        map_value_iterator<M>(m.end()));
}

template <typename R, typename It>
    requires can_push_back<R>
inline R collect(It begin, It end, R &rs) {
    while (begin != end) {
        rs.push_back(*begin);
        begin++;
    }
    return rs;
}

template <typename R, typename Q>
    requires can_push_back<R>
inline R collect(const Q &qs, R &rs) {
    return collect(qs.begin(), qs.end(), rs);
}

template <typename R, typename Q>
    requires can_push_back<R>
inline R collect(const Q &qs) {
    R rs;
    return collect(qs.begin(), qs.end(), rs);
}

template <typename R, typename Q, typename F>
    requires can_push_back<R>
inline R collect_if(const Q &qs, R &rs, F &&f) {
    for (const auto &q : qs) {
        if (f(q)) {
            rs.push_back(q);
        }
    }
    return rs;
}

template <typename R, typename Q, typename F>
inline R collect_if(const Q &qs, F &&f) {
    R rs;
    return collect_if(qs, rs, std::forward<F>(f));
}

template <typename R, typename M>
    requires (
        can_push_back<R>
        && is_map<M>)
inline R collect_keys(const M &m, R &rs) {
    return collect(
        map_key_iterator<M>(m.begin()),
        map_key_iterator<M>(m.end()),
        rs);
}

template <typename R, typename M>
inline R collect_keys(const M &m) { R rs; return collect_keys(m, rs); }

template <typename R, typename M>
    requires (can_push_back<R> && is_map<M>)
inline R collect_values(const M &m, R &rs) {
    return collect(
        map_value_iterator<M>(m.begin()),
        map_value_iterator<M>(m.end()),
        rs);
}

template <typename R, typename M>
inline R collect_values(const M &m) { R rs; return collect_values(m, rs); }

template <typename R, typename Q, typename F, typename B = typename Q::value_type>
    requires
        (can_push_back<R>
        && (requires (R rs, F f, const typename Q::value_type &v) {
            { f(v) } -> std::convertible_to<typename R::value_type>;
        }))
inline R transform(const Q &qs, R &rs, F &&f) {
    for (const auto &q : qs) {
        rs.push_back(f(q));
    }
    return rs;
}

template <typename R, typename Q, typename F>
inline R transform(const Q &qs, F &&f) {
    R rs;
    return transform(qs, rs, std::forward<F>(f));
}

template <typename R, typename Q, typename F, typename G>
    requires
        (can_push_back<R>
        && (requires (F f, const typename Q::value_type &v) {
            { f(v) } -> std::same_as<bool>;
        })
        && (requires (G g, const typename Q::value_type &v) {
            { g(v) } -> std::convertible_to<typename R::value_type>;
        }))
inline R transform_if(const Q &qs, R &rs, F &&f, G &&g) {
    for (const auto &q : qs) {
        if (f(q)) {
            rs.push_back(g(q));
        }
    }
    return rs;
}

template <typename R, typename Q, typename F, typename G>
inline R transform_if(const Q &qs, F &&f, G &&g) {
    R rs;
    return transform_if(qs, rs, std::forward<F>(f), std::forward<G>(g));
}

template <typename R, typename Q, typename F>
    requires
        (can_push_back<R>
        && (requires (F f, const typename Q::value_type &v) {
                { f(v) } -> std::same_as<bool>;
            }))
inline R filter(const Q &qs, R &rs, F &&f) {
    for (const auto &q : qs) {
        if (f(q)) {
            rs.push_back(q);
        }
    }
    return rs;
}

template <typename R, typename Q, typename F>
    requires
        (can_push_back<R>
        && (requires (F f, const typename Q::value_type &v) {
                { f(v) } -> std::same_as<bool>;
            }))
inline R filter(const Q &qs, F &&f) {
    R rs;
    return filter(qs, rs, std::forward<F>(f));
}

template <typename Q, typename F, typename T = typename Q::value_type>
inline std::optional<T> find(const Q &qs, F &&f) {
    auto it = std::find_if(qs.begin(), qs.end(), std::forward<F>(f));
    return it == qs.end() ? std::nullopt : std::make_optional(*it);
}

template <
    typename T,
    typename F,
    typename U = std::invoke_result_t<F, const T&>>
inline std::optional<U> map_opt(const std::optional<T> &t, F &&f) {
    return t ? std::make_optional(f(*t)) : std::nullopt;
}

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <
    typename T,
    typename F,
    typename U = std::invoke_result_t<F, const T&>>
    requires is_optional<U>::value
inline U flat_map_opt(const std::optional<T> &t, F &&f) {
    return t ? f(*t) : std::nullopt;
}

template <typename Q, typename F, typename T = typename Q::value_type>
    requires std::same_as<std::invoke_result_t<F, const T&>, bool>
inline bool all_of(const Q &qs, F &&f) {
    return std::all_of(qs.begin(), qs.end(), f);
}

template <typename Q, typename F, typename T = typename Q::value_type>
    requires std::same_as<std::invoke_result_t<F, const T&>, bool>
inline bool any_of(const Q &qs, F &&f) {
    return std::any_of(qs.begin(), qs.end(), f);
}
} // namespace detail
} // namespace archimedes
