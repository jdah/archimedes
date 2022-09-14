#pragma once

#include <type_traits>
#include <cstdlib>
#include <utility>
#include <span>

#include "errors.hpp"
#include "type_id.hpp"

#include <iostream>

namespace archimedes {
struct reflected_type;

// generic storage for any type
// TODO: optimize so types with size < sizeof(void*) don't heap allocate
// TODO: store info (functions, etc.) in another structure that is not
// duplicated per-any
struct any {
    any() = default;

    any(const any &other) {
        *this = other;
    }

    any &operator=(const any& other) {
        // clean up existing data
        this->~any();

        this->_id = other._id;
        this->data_size = other.data_size;
        this->data =
            this->data_size ? std::malloc(this->data_size) : other.data;
        this->is_data_owned = this->data_size != 0;
        this->copy_fn = other.copy_fn;
        this->dtor_fn = other.dtor_fn;

        if (this->data_size != 0) {
            if (!this->copy_fn) {
                ARCHIMEDES_FAIL("attempt to copy uncopyable type");
            }

            (*this->copy_fn)(this->data, other.data);
        }

        return *this;
    }

    any(any&& other) {
        *this = std::move(other);
    }

    any &operator=(any&& other) {
        // clean up existing data
        this->~any();

        this->_id = other._id;
        this->data_size = other.data_size;
        this->data = other.data;
        this->is_data_owned = other.is_data_owned;
        this->copy_fn = std::move(other.copy_fn);
        this->dtor_fn = std::move(other.dtor_fn);
        other.data = nullptr;
        other.is_data_owned = false;

        return *this;
    }

    ~any();

    // get id of underlying type
    type_id id() const {
        return this->_id;
    }

    // TODO: remove
    // NOTE: this is dangerous as ptr/ref/etc. types are stored *directly* as a
    // void pointer. only use this if you know what you're doing!
    void *storage() const {
        return this->data;
    }

    // get data underlying any
    std::span<uint8_t> bytes() const {
        if (this->data_size == 0) {
            return std::span<uint8_t>(
                reinterpret_cast<uint8_t*>(
                    const_cast<void**>(&this->data)),
                sizeof(this->data));
        } else {
            return std::span<uint8_t>(
                reinterpret_cast<uint8_t*>(
                    const_cast<void*>(this->data)),
                this->data_size);
        }
    }

    // size of storage of this any in bytes
    size_t size() const {
        return this->data_size == 0 ? sizeof(this->data) : this->data_size;
    }

    // returns true if this any is an instance of T
    template <typename T>
    bool is() const {
        return this->_id == type_id::from<T>();
    }

    // cast underlying value to T
    template <typename T>
    T as() const {
        if constexpr (std::is_rvalue_reference_v<T>) {
            return std::move(
                *reinterpret_cast<
                    std::add_pointer_t<
                        std::decay_t<T>>>(this->data));
        } else if constexpr (std::is_reference_v<T>) {
            return
                *reinterpret_cast<
                    std::add_pointer_t<
                        std::remove_reference_t<T>>>(this->data);
        } else if constexpr (std::is_pointer_v<T>) {
            return reinterpret_cast<T>(this->data);
        } else {
            return *reinterpret_cast<T*>(this->data);
        }
    }

    // get any which is pointer to this any
    // if this is a ptr/ref: actually make a void** which is the value
    // if this is a value: make an any void* which points to our data
    any ptr(type_id id = type_id::none()) const {
        return any::make<const void*>(
            this->data_size == 0 ?
                reinterpret_cast<const void*>(&this->data)
                : this->data,
            id ? id : this->id().add_pointer());
    }

    // create for some reflected type
    // initialized with default constructor, returns nullopt if there is no
    // default ctor (see runtime/any.cpp for impl)
    static result<any, any_error> make_for_id(type_id id);

    // create for some reflected type
    // initialized with default constructor, returns nullopt if there is no
    // default ctor
    static result<any, any_error> make_for_type(
        const reflected_type &type);

    // create any from const reference via copy
    // value types which can't be std::move'd get copied
    template <typename T>
        requires (
            !std::is_array_v<T>
            && !std::is_pointer_v<T>
            && !(std::is_reference_v<T> && std::is_pointer_v<std::decay_t<T>>)
            && !(std::is_reference_v<T> && std::is_array_v<std::decay_t<T>>)
            && std::is_copy_constructible_v<T>)
    static inline any make(const T &t, type_id id = type_id::none()) {
        any a = make_of_size(
            id ? id : type_id::from<T>(),
            sizeof(T),
            get_copy<T>(),
            get_dtor<T>());
        new (a.data) T(t);
        return a;
    }

    // create any from rvalue reference
    // value types which can be std::move'd get std::move'd
    template <typename T>
        requires (
            !std::is_array_v<T>
            && !std::is_pointer_v<T>
            && !(std::is_reference_v<T> && std::is_pointer_v<std::decay_t<T>>)
            && !(std::is_reference_v<T> && std::is_array_v<std::decay_t<T>>)
            && std::is_move_constructible_v<T>
            && (std::is_rvalue_reference_v<T>
                    || std::is_same_v<T, std::remove_reference_t<T>>))
    static inline any make(T &&t, type_id id = type_id::none()) {
        using U = std::remove_const_t<std::remove_reference_t<T>>;
        any a = make_of_size(
            id ? id : type_id::from<U>(),
            sizeof(T),
            get_copy<U>(),
            get_dtor<U>());
        new (a.data) T(std::move(t));
        return a;
    }

    // create any from pointer
    template <typename T>
        requires (std::is_pointer_v<T> || std::is_array_v<T>)
    static inline any make(T t, type_id id = type_id::none()) {
        if constexpr (std::is_array_v<T>) {
            // arrays are converted implicitly to pointers
            return make(&t[0], id ? id : type_id::from<T>());
        }

        any a = make_ptr(id ? id : type_id::from<T>());
        a.data = const_cast<void*>(reinterpret_cast<const void *>(t));
        return a;
    }

    // references must be stored *explicitly* if the data value is to keep its
    // reference qualifiers. otherwise the value is std::move'd or copied
    template <typename T>
    static inline any make_reference(T &t, type_id id = type_id::none()){
        return make(&t, id ? id : type_id::from<T&>());
    }

    // create reference any
    template <typename T>
    static inline any make_reference(const T &t, type_id id = type_id::none()) {
        return make(&t, id ? id : type_id::from<const T&>());
    }

    // create reference any from ptr (creates a T& from T*, NOT T*& from T*)
    static inline any make_reference_from_ptr(void *ptr, type_id id) {
        return make(ptr, id);
    }

    // create none/nil value any
    static inline any none() {
        return any();
    }

protected:
    template <typename T>
    static std::optional<std::function<void(void*, void*)>> get_copy() {
        if constexpr (std::is_copy_constructible_v<T>) {
            return [](void *p, void *o) {
                new (p) T(*reinterpret_cast<T*>(o));
            };
        } else if constexpr (
            std::is_copy_assignable_v<T>
            && std::is_default_constructible_v<T>) {
            return [](void *p, void *o) {
                new (p) T();
                *reinterpret_cast<T*>(p) = *reinterpret_cast<T*>(o);
            };
        }

        return std::nullopt;
    }

    template <typename T>
    static std::optional<std::function<void(void*)>> get_dtor() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            return [](void *p) { reinterpret_cast<T*>(p)->~T(); };
        }

        return std::nullopt;
    }

    // make any with arbitrarily sized storage
    static inline any make_of_size(
        type_id id,
        size_t data_size = 0,
        std::optional<std::function<void(void*, void*)>> copy = std::nullopt,
        std::optional<std::function<void(void*)>> dtor = std::nullopt) {
        any a;
        a._id = id;
        a.data_size = data_size;
        a.copy_fn = copy;
        a.dtor_fn = dtor;

        if (a.data_size != 0) {
            a.data = std::malloc(data_size);
            a.is_data_owned = true;
        } else {
            a.is_data_owned = false;
        }

        return a;
    }

    // make any with only data in pointer
    static inline any make_ptr(type_id id) {
        any a;
        a._id = id;
        a.data_size = 0;
        a.copy_fn = std::nullopt;
        a.dtor_fn = std::nullopt;
        a.is_data_owned = false;
        return a;
    }

    // type_id<T> of stored value
    type_id _id = type_id::none();

    // size of data buffer, 0 if data is stored directly in ptr
    size_t data_size = 0;

    // data, may not be valid pointer
    void *data = nullptr;

    // if true, data must be free'd
    bool is_data_owned = false;

    // copy/move from arg 1 to arg 0
    std::optional<std::function<void(void*, void*)>> copy_fn;

    // destructor
    std::optional<std::function<void(void*)>> dtor_fn;
};
} // end namespace archimedes
