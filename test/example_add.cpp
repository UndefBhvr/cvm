#include "../include/icm_core.hpp"
#include <cstdio>

int main() {
    printf("=== ICM Software Simulator - a+b Example ===\n\n");

    icm::ICMSimulator sim;

    // Create NUM(3) for value a
    size_t num_a = sim.rexu().create_num(icm::NumType::LIT, 3);
    printf("Created NUM(3) at address %zu\n", num_a);

    // Create NUM(5) for value b
    size_t num_b = sim.rexu().create_num(icm::NumType::LIT, 5);
    printf("Created NUM(5) at address %zu\n", num_b);

    // Create OPR(left, right) for addition
    // OPR's main port = left operand, aux1 = right operand
    size_t opr = sim.rexu().create_opr(num_a, num_b, "+");
    printf("Created OPR(+) at address %zu with left=%zu, right=%zu\n", opr, num_a, num_b);

    // Wire OPR's principal port to NUM(a) - this forms the redex for reduction
    // NUM(a) ~ OPR => OPER reduction
    sim.add_wire(opr, 0, num_a, 0);
    printf("Wire created: OPR.principal <-> NUM(3)\n");

    printf("\nInitial state before reduction:\n");
    sim.print_state();
    sim.print_wires();

    printf("\nRunning reductions...\n");
    sim.run();

    printf("\nFinal state after %d reductions:\n", sim.get_reduction_count());
    sim.print_state();

    // Check the result - OPR should have been replaced by NUM with result
    const icm::RedexRegister& result_reg = sim.rexu().get_register(opr);
    icm::CombinatorType type = static_cast<icm::CombinatorType>(result_reg.metadata & 0xFFFF);
    if (type == icm::CombinatorType::NUM) {
        printf("\n=== Result ===\n");
        printf("Expression: 3 + 5 = %u\n", result_reg.port_main);
    }

    return 0;
}