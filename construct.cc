#include <iostream>

template <size_t N, typename... Types>
struct construct_impl {
};

template <typename... Args>
struct construct {
    construct_impl<sizeof(Args...), Args...>::
};


int main() {
    construct<A, B, C>::type;
    return 0;
}
