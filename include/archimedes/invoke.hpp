#pragma once

#include <span>
#include "any.hpp"

namespace archimedes {
struct invoke_result;
namespace detail {
// signature for generated invoker functions
using invoker_ptr = invoke_result(*)(std::span<archimedes::any>);
}

// result of function invoke
struct invoke_result {
    enum Enum {
        SUCCESS = 0,
        BAD_ARG_COUNT = 1,
        NO_ACCESS = 2
    };

    invoke_result() = default;

    // intentional implicit conversion
    invoke_result(Enum status, any &&result = any::none())
        : _status(status),
          _result(std::move(result)) {}

    bool is_success() const {
        return this->_status == SUCCESS;
    }

    operator bool() const {
        return this->is_success();
    }

    Enum status() const {
        return this->_status;
    }

    const any &result() const {
        return this->_result;
    }

    any &result() {
        return this->_result;
    }

    const any &operator*() const {
        return this->_result;
    }

    any &operator*() {
        return this->_result;
    }

    auto operator->() const {
        return &**this;
    }

    auto operator->() {
        return &**this;
    }

    // void success
    static inline auto success() {
        return invoke_result(SUCCESS);
    }

    static inline auto success(any &&a) {
        return invoke_result(SUCCESS, std::move(a));
    }

private:
    Enum _status;
    any _result;
};
}
