#include "test.hpp"
#include "invoke.test.hpp"

std::string &i_return_my_argument(std::string &s) {
    s.append("aaa");
    return s;
}

std::string &i_return_something_new(std::string &s) {
    static std::string q = s;
    q.append("aaa");
    return q;
}

int lots_of_args(int x, int y, int z, const std::string &p, std::string &&q) {
    std::string f = std::move(q);
    return x + y + z + p.length() + f.length();
}

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo>();
    ASSERT(f);
    const auto ssm = f->as_record().function("some_static_method");
    ASSERT(ssm);
    ASSERT(ssm->invoke(4)->as<int>() == 6);

    const auto wrap = [&](const int &x) { return ssm->invoke(x)->as<int>(); };
    ASSERT(wrap(12) == 14);

    const auto nsm = f->as_record().function("another_static_method");
    ASSERT(nsm);
    auto iccm = ICountMyMoves();
    ASSERT(nsm->invoke(archimedes::rref(std::move(iccm)))->as<int>() == 1);

    const auto irma =
        archimedes::reflect_functions("i_return_my_argument").begin()->first();
    ASSERT(irma);
    ASSERT(irma->can_invoke());
    std::string s = "bbb";
    std::string &t = irma->invoke(archimedes::ref(s))->as<std::string&>();
    ASSERT(&s == &t);
    ASSERT(s == "bbbaaa");
    ASSERT(t == "bbbaaa");

    const auto irsn =
        archimedes::reflect_functions("i_return_something_new").begin()->first();
    std::string q = "bbb";
    std::string &r = irsn->invoke(archimedes::ref(q))->as<std::string&>();
    ASSERT(&r != &q);
    ASSERT(q == "bbb");
    ASSERT(r == "bbbaaa");

    const auto loa =
        archimedes::reflect_functions("lots_of_args").begin()->first();
    ASSERT(loa);

    std::string a = "aaa";
    std::string b = "b";
    ASSERT(
        loa->invoke(
            1, 2, 3,
            archimedes::cref(a),
            archimedes::rref(std::move(b)))->as<int>()
            == 10);

    const auto ial = archimedes::reflect_functions("i_am_inline").begin()->first();
    ASSERT(ial);
    ASSERT(ial->can_invoke());
    ASSERT(ial->invoke(4)->as<int>() == 16);

    const auto maiv = archimedes::reflect_functions("my_arg_is_void").begin()->first();
    ASSERT(maiv);
    ASSERT(maiv->can_invoke());
    maiv->invoke();
    ASSERT(maiv->invoke()->as<std::string>() == "xyz");

    Foo foo;
    const auto inc_w = f->as_record().function("inc_w");
    const auto member_c = f->as_record().function("member_c");
    const auto member_cr = f->as_record().function("member_cr");
    const auto member_rr = f->as_record().function("member_rr");

    foo.w = 200;
    ASSERT(inc_w->invoke(&foo, 3).is_success());
    ASSERT(foo.w == 201);

    foo.w = 21;
    ASSERT(member_c->invoke(const_cast<const Foo*>(&foo), 2)->as<int>() == 23);
    ASSERT(member_cr->invoke(archimedes::cref(foo), 2)->as<int>() == 25);
    ASSERT(member_rr->invoke(std::move(foo), 2)->as<int>() == 27);

    return 0;
}
