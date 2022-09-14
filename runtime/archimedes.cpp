#include <archimedes.hpp>
#include <archimedes/type_info.hpp>

using namespace archimedes;
using namespace archimedes::detail;

template struct
    archimedes::name_map<archimedes::detail::function_overload_set>;
template struct
    archimedes::name_map<archimedes::detail::function_parameter_info>;
template struct
    archimedes::name_map<archimedes::detail::static_field_type_info>;
template struct
    archimedes::name_map<archimedes::detail::field_type_info>;
template class std::vector<std::string>;

const type_info &type_id::operator*() const {
    return **registry::instance().type_from_id(*this);
}

type_id::operator bool() const {
    return (*this) != none()
        && registry::instance().type_from_id(*this);
}
