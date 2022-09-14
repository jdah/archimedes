#include <archimedes/cast.hpp>
#include <archimedes/types.hpp>
#include <archimedes/ds.hpp>
#include <archimedes.hpp>

// TODO: remove
#include <iostream>

using namespace archimedes;
using namespace archimedes::detail;

// get byte difference between a cast from a ptr_t to type "to"
result<std::ptrdiff_t, cast_error> archimedes::detail::cast_diff_impl(
    const any &ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to,
    bool only_up) {
    if (from == to) {
        // identity cast
        return 0;
    }

    void *ptr = ptr_t.as<void*>();

    // check if to is a vbase of from
    const auto vbases = from.vbases();
    auto it =
        std::find_if(
            vbases.begin(), vbases.end(),
            [&to](const reflected_base &vbase) {
                return vbase.type() == to;
            });

    // cast directly to vbase
    if (it != vbases.end()) {
        return
            reinterpret_cast<char*>(it->cast_up(ptr))
                - reinterpret_cast<char*>(ptr);
    }

    // try casting from regular bases, find the first one that succeeds
    for (const auto &base : from.bases()) {
        // try directly casting up and continuing with upcasts. add the offset
        // from this upcast to the total with *res.
        //
        // deliberately ignore errors, we'll just NOT FOUND in the end if
        // nothing is found this way.
        //
        // also ONLY UPCAST, otherwise we start to get some strange paths to
        // "to"...
        auto *ptr_base = base.cast_up(ptr);
        if (const auto res =
                cast_diff_impl(
                    any::make<void*>(ptr_base),
                    base.type(),
                    to,
                    true)) {
            return
                *res
                    + (reinterpret_cast<char*>(ptr_base)
                        - reinterpret_cast<char*>(ptr));
        }
    }

    // only up cast and here, there is no cast
    if (only_up) {
        return cast_error::NOT_FOUND;
    }

    // check if from is above to in the inheritance hierarchy (downcast)
    // use a standard depth first search (reimplemented since traverse_bases is
    // not adequate) to find a path from to -> from
    vector<reflected_base> path, bases;
    map<type_id, bool> visited;

    std::function<void(const reflected_base&)> dfs;
    dfs =
        [&](const reflected_base &base) {
            path.push_back(base);

            // check if already visited
            auto it = visited.find(base.type().id());
            if (it == visited.end() || !it->second) {
                visited[base.type().id()] = true;
            } else {
                return;
            }

            if (base.type() == from) {
                bases = path;
            } else {
                for (const auto &b : base.type().bases()) {
                    auto it = visited.find(b.type().id());
                    if (it == visited.end() || !it->second) {
                        dfs(b);
                    }
                }
            }

            visited[base.type().id()] = false;
            path.pop_back();
        };

    for (const auto &base : to.all_bases()) {
        dfs(base);
    }

    if (!bases.empty()) {
        // go backwards through path casting down
        void *out = const_cast<void*>(ptr);
        for (int i = bases.size() - 1; i >= 0; i--) {
            out = bases[i].cast_down(out);
        }
        return reinterpret_cast<char*>(out) - reinterpret_cast<char*>(ptr);
    }

    return cast_error::NOT_FOUND;
}

result<cast_fn, cast_error> archimedes::detail::make_cast_fn_impl(
    const any &ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to) {
    const auto diff = cast_diff_impl(ptr_t, from, to);
    if (diff) {
        return [diff = *diff](void *p) -> void* {
            return reinterpret_cast<char*>(p) + diff;
        };
    } else {
        return diff.unwrap_error();
    }
}

result<void*, cast_error> archimedes::detail::cast_impl(
    const any &ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to) {
    const auto fn = make_cast_fn_impl(ptr_t, from, to);
    if (fn) {
        return (*fn)(ptr_t.as<void*>());
    } else {
        return fn.unwrap_error();
    }
}
