#include <archimedes/any.hpp>
#include <archimedes.hpp>

#include <iostream>

using namespace archimedes;

any::~any() {
    if (this->is_data_owned) {
        // check if archimedes is still loaded - if it isn't, our dtor function
        // may have disappeared.
        if (archimedes::loaded() && this->dtor_fn) {
            (*this->dtor_fn)(this->data);
        }

        std::free(this->data);
    }
}

result<any, any_error> any::make_for_id(type_id id) {
    const auto type = reflect(id);
    if (!type) {
        return any_error::COULD_NOT_REFLECT;
    }

    const auto kind = type->kind();

    if (kind == VOID
        || kind == UNKNOWN
        || kind == FUNC) {
        return any_error::INVALID_TYPE;
    } else if (kind == PTR
        || kind == REF
        || kind == RREF
        || kind == MEMBER_PTR) {
        // return raw any, data is stored directly in pointer
        return any::make_ptr(id);
    }

    any a = any::make_of_size(id, type->size());

    if (type->is_record()) {
        if (type->size() == 0) {
            return any_error::INVALID_TYPE;
        }

        const auto rec = type->as_record();
        const auto ctor = rec.default_constructor();
        if (!ctor) {
            return any_error::NO_DEFAULT_CTOR;
        }

        if (const auto dtor = rec.destructor()) {
            a.dtor_fn =
                [dtor = *dtor](void *p) {
                    dtor.invoke(p);
                };
        }

        if (const auto copy = rec.copy_constructor()) {
            a.copy_fn =
                [copy = *copy](void *p, void *o) {
                    if (!copy.invoke(p, o)) {
                        ARCHIMEDES_FAIL("failed to invoke copy ctor");
                    }
                };
        } else if (const auto copy_assign = rec.copy_assignment()) {
            a.copy_fn =
                [ctor = *ctor, copy_assign = *copy_assign](void *p, void *o) {
                    if (!ctor.invoke(p)) {
                        ARCHIMEDES_FAIL("failed to invoke default ctor");
                    }

                    if (!copy_assign.invoke(p, o)) {
                        ARCHIMEDES_FAIL("failed to invoke copy assign");
                    }
                };
        }

        if (!ctor->invoke(a.storage())) {
            ARCHIMEDES_FAIL("failed to invoke default ctor");
        }
    } else if (type->is_numeric()) {
        // numeric types are stored on the heap with a simple memcpy copy
        a.copy_fn =
            [size = a.data_size](void *p, void *o) {
                std::memcpy(p, o, size);
            };
        std::memset(a.data, 0, a.data_size);
    }

    return a;
}

result<any, any_error> any::make_for_type(
    const reflected_type &type) {
    return any::make_for_id(type.id());
}
