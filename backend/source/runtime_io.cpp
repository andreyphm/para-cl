#include <cstdlib>
#include <iostream>

#ifdef _WIN32
    #define PARACL_RUNTIME_EXPORT __declspec(dllexport)
#else
    #define PARACL_RUNTIME_EXPORT
#endif

extern "C" PARACL_RUNTIME_EXPORT int paracl_input() {
    int value = 0;
    std::cin >> value;
    if (!std::cin) {
        std::cerr << "paracl_input error" << "\n";
        std::exit(EXIT_FAILURE);
    }
    return value;
}

extern "C" PARACL_RUNTIME_EXPORT void paracl_output(int value) {
    std::cout << value << "\n";
    if (!std::cout) {
        std::cerr << "paracl_output error" << "\n";
        std::exit(EXIT_FAILURE);
    }
}
