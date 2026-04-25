#include "../include/icm_core.hpp"
#include <cstdio>

// Pure ICM Sparse Matrix-Vector Multiplication (4x4)
// All computation via ICM reduction rules

constexpr int MAT_SIZE = 4;
size_t g_result_regs[MAT_SIZE];

size_t create_mul_chain(icm::ICMSimulator& sim, int init_val, int times) {
    if (times <= 0) return sim.rexu().create_num(icm::NumType::LIT, 0);

    size_t nums[32];
    for (int i = 0; i < times; i++) {
        nums[i] = sim.rexu().create_num(icm::NumType::LIT, init_val);
    }

    size_t current = nums[0];
    struct WireInfo { size_t opr; size_t target; };
    WireInfo wires[32];
    int wc = 0;

    for (int i = 1; i < times; i++) {
        size_t opr = sim.rexu().create_opr(current, nums[i], "+");
        wires[wc++] = {opr, current};
        current = opr;
    }

    for (int i = wc - 1; i >= 0; i--) {
        sim.add_wire(wires[i].opr, 0, wires[i].target, 0);
    }

    return current;
}

size_t create_add_chain(icm::ICMSimulator& sim, size_t* terms, int count) {
    if (count == 0) return sim.rexu().create_num(icm::NumType::LIT, 0);
    if (count == 1) return terms[0];

    size_t result = sim.rexu().create_opr(terms[0], terms[1], "+");
    sim.add_wire(result, 0, terms[0], 0);

    for (int i = 2; i < count; i++) {
        size_t new_result = sim.rexu().create_opr(result, terms[i], "+");
        sim.add_wire(new_result, 0, result, 0);
        result = new_result;
    }

    return result;
}

int main() {
    printf("========================================\n");
    printf("Pure ICM Sparse Matrix-Vector Multiplication\n");
    printf("========================================\n\n");

    printf("Sparse matrix A (4x4):\n");
    printf("  [1, 0, 0, 2]\n");
    printf("  [0, 0, 3, 0]\n");
    printf("  [0, 4, 0, 0]\n");
    printf("  [5, 0, 0, 0]\n\n");
    printf("Vector x = [1, 2, 3, 4]\n\n");
    printf("Expected: y[0]=9, y[1]=9, y[2]=8, y[3]=5\n\n");

    int x[MAT_SIZE] = {1, 2, 3, 4};

    icm::ICMSimulator sim;

    printf("Building ICM network:\n\n");

    // Create x vector registers
    printf("  Creating x vector registers...\n");
    for (int i = 0; i < MAT_SIZE; i++) {
        sim.rexu().create_num(icm::NumType::LIT, x[i]);
    }

    // Build all multiplication chains and run after each
    // Row 0: A[0][0]*x[0] = 1*1, A[0][3]*x[3] = 2*4
    printf("  Row 0 mul chains:\n");
    size_t r0_prod0 = create_mul_chain(sim, 1, x[0]);
    printf("    A[0][0]*x[0] = 1*%d at reg %zu, running...\n", x[0], r0_prod0);
    sim.run();
    printf("    result: %u\n", sim.rexu().get_register(r0_prod0).port_main);

    size_t r0_prod1 = create_mul_chain(sim, 2, x[3]);
    printf("    A[0][3]*x[3] = 2*%d at reg %zu, running...\n", x[3], r0_prod1);
    sim.run();
    printf("    result: %u\n", sim.rexu().get_register(r0_prod1).port_main);

    // Row 1: A[1][2]*x[2] = 3*3
    printf("  Row 1 mul chain:\n");
    size_t r1_prod0 = create_mul_chain(sim, 3, x[2]);
    printf("    A[1][2]*x[2] = 3*%d at reg %zu, running...\n", x[2], r1_prod0);
    sim.run();
    printf("    result: %u\n", sim.rexu().get_register(r1_prod0).port_main);

    // Row 2: A[2][1]*x[1] = 4*2
    printf("  Row 2 mul chain:\n");
    size_t r2_prod0 = create_mul_chain(sim, 4, x[1]);
    printf("    A[2][1]*x[1] = 4*%d at reg %zu, running...\n", x[1], r2_prod0);
    sim.run();
    printf("    result: %u\n", sim.rexu().get_register(r2_prod0).port_main);

    // Row 3: A[3][0]*x[0] = 5*1
    printf("  Row 3 mul chain:\n");
    size_t r3_prod0 = create_mul_chain(sim, 5, x[0]);
    printf("    A[3][0]*x[0] = 5*%d at reg %zu, running...\n", x[0], r3_prod0);
    sim.run();
    printf("    result: %u\n", sim.rexu().get_register(r3_prod0).port_main);

    // Build sum for row 0
    printf("  Building row 0 sum (y[0] = prod0 + prod1)...\n");
    size_t r0_terms[2] = {r0_prod0, r0_prod1};
    g_result_regs[0] = create_add_chain(sim, r0_terms, 2);
    printf("    y[0] sum at reg %zu, running...\n", g_result_regs[0]);
    sim.run();
    printf("    result: %u\n", sim.rexu().get_register(g_result_regs[0]).port_main);

    // Rows 1, 2, 3 are single products - no sum needed
    g_result_regs[1] = r1_prod0;
    g_result_regs[2] = r2_prod0;
    g_result_regs[3] = r3_prod0;

    printf("\n  Total reductions: %d\n", sim.get_reduction_count());

    printf("\n--- Results ---\n");
    for (int i = 0; i < MAT_SIZE; i++) {
        int val = sim.rexu().get_register(g_result_regs[i]).port_main;
        printf("  y[%d] = %d", i, val);
        switch(i) {
            case 0: printf(" (expected 9)"); break;
            case 1: printf(" (expected 9)"); break;
            case 2: printf(" (expected 8)"); break;
            case 3: printf(" (expected 5)"); break;
        }
        printf("\n");
    }

    bool pass = (sim.rexu().get_register(g_result_regs[0]).port_main == 9 &&
                 sim.rexu().get_register(g_result_regs[1]).port_main == 9 &&
                 sim.rexu().get_register(g_result_regs[2]).port_main == 8 &&
                 sim.rexu().get_register(g_result_regs[3]).port_main == 5);

    printf("\n  Status: %s\n", pass ? "PASS" : "FAIL");

    return pass ? 0 : 1;
}