#include "test.hpp"
#include "lambda.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    std::vector<int> xs = { 1, 2, 3, 4, 5 };
    int sum = 0;
    each(xs, [&](const auto &i) { sum += i; });
    return 0;
}
