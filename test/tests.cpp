// archimedes test runner
// usage: $ tests [tests directory]
// looks for executables in test directory and runs them, failing if any of the
// executables return 1 (error)

#include <string>
#include <iostream>
#include <filesystem>
#include <cstdlib>

#define FMT_HEADER_ONLY
#include <fmt/core.h>

template <typename ...Args>
static std::string _assert_fmt(
    const std::string &fmt = "",
    Args&&... args) {
    if (fmt.length() > 0) {
        return fmt::vformat(
            std::string_view(fmt),
            fmt::make_format_args(std::forward<Args>(args)...));
    }

    return "";
}

#define ASSERT(_e, ...) do {                                                   \
        if (!(_e)) {                                                           \
            const auto __msg = _assert_fmt(__VA_ARGS__);                       \
            fmt::print(                                                        \
                "ASSERTION FAILED{}{}\n",                                      \
                __msg.length() > 0 ? " " : "", __msg);                         \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    ASSERT(argc >= 2, "expected argc >= 2");

    std::vector<std::string> explicit_tests;
    for (int i = 2; i < argc; i++) {
        explicit_tests.push_back(argv[i]);
    }

    const auto tests_dir = fs::path(argv[1]);
    ASSERT(fs::is_directory(tests_dir), "argv[1] must be directory");

    // format strings for good things/bad things
    std::string fmt_good = "{}", fmt_bad = "{}";

    // check if terminal probably supports ANSI color codes
    if (std::getenv("TERM") != nullptr) {
        fmt_good = "\033[32;1m{}\033[0m";
        fmt_bad = "\033[31;1m{}\033[0m";
    }

    // execute command, return (exit code, output)
    const auto exec =
        [](std::string_view cmd) -> std::tuple<int, std::string> {
            FILE *pipe = popen(&cmd[0], "r");

            if (!pipe) {
                return std::make_tuple(1, "");
            }

            std::string out;
            char c;
            while ((c = std::fgetc(pipe)) != EOF) {
                out += c;
            }

            return std::make_tuple(pclose(pipe), out);
        };

    // true if any test failed
    bool fail = false;

    for (const auto &e : fs::directory_iterator(tests_dir)) {
        const auto &p = e.path();

        if (!fs::exists(p)
                || fs::is_directory(p)
                || p.has_extension()
                || ((fs::status(p).permissions()
                        & (fs::perms::owner_exec
                           | fs::perms::group_exec
                           | fs::perms::others_exec)) == fs::perms::none)
                || fs::equivalent(fs::path(argv[0]), p)) {
            continue;
        }

        // check if explicit tests specified
        if (!explicit_tests.empty()
                && !std::any_of(
                    explicit_tests.begin(),
                    explicit_tests.end(),
                    [&](const auto &name) {
                        return name == p.filename().string();
                    })) {
            continue;
        }

        fmt::print("{} ... ", p.filename().string());
        const auto [exit_code, out] =
            exec(fmt::format("./{} 2>&1", p.string()));

        if (!exit_code) {
            fmt::print(
                "{}\n{}",
                fmt::format(
                    fmt::runtime(fmt_good), "[OK]"),
                (out.empty()
                    || std::all_of(
                        out.begin(),
                        out.end(),
                        [](char c) { return std::isspace(c); })) ?
                    "" : out);
        } else {
            fmt::print(
                "{}\n{}",
                fmt::format(
                    fmt::runtime(fmt_bad),
                    "[FAIL]"),
                out);
            fail = true;
        }
    }

    if (fail) {
        fmt::print(
            "{}\n",
            fmt::format(
                fmt::runtime(
                    fmt_bad),
                "TEST(S) FAILED"));
        return 1;
    } else {
        fmt::print(
            "{}\n",
            fmt::format(
                fmt::runtime(
                    fmt_good),
                "ALL TESTS PASSED"));
        return 0;
    }
}
