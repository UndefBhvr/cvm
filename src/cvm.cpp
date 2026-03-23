#include "define.hpp"
#include <iostream>

int main() {
    // Example usage of the Combinator class
    Combinator c1 = Combinator::CON;
    Combinator c2 = Combinator::OPX;
    
    // Output the types of the combinators
    std::cout << "Combinator c1 type: " << static_cast<int>(c1.type()) << std::endl;
    std::cout << "Combinator c2 type: " << static_cast<int>(c2.type()) << std::endl;

    return 0;
}