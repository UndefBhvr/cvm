#include "../include/icm_core.hpp"
#include <cstdio>

// Pure ICM Matrix-Vector Multiplication
// Key: Build one chain, run() to completion, THEN build next chain

constexpr int MAT_SIZE = 2;
size_t g_result_regs[MAT_SIZE];

size_t create_mul_chain(icm::ICMSimulator& sim, int init_val, int times) {
    if (times <= 0) {
        return sim.rexu().create_num(icm::NumType::LIT, 0);
    }

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

    // Add wires in REVERSE order
    for (int i = wc - 1; i >= 0; i--) {
        sim.add_wire(wires[i].opr, 0, wires[i].target, 0);
    }

    return current;
}

size_t create_add_chain(icm::ICMSimulator& sim, size_t* terms, int count, const char* name) {
    if (count == 0) {
        return sim.rexu().create_num(icm::NumType::LIT, 0);
    }
    if (count == 1) {
        return terms[0];
    }

    // Create all OPRs first
    size_t oprs[16];
    for (int i = 0; i < count - 1; i++) {
        oprs[i] = sim.rexu().create_opr(0, 0, "+");  // placeholder
    }

    // Set left/right operands
    auto reg0 = sim.rexu().get_register(oprs[0]);
    reg0.port_main = terms[0];
    reg0.port_aux1 = terms[1];
    sim.rexu().set_register(oprs[0], reg0);

    for (int i = 1; i < count - 1; i++) {
        auto reg = sim.rexu().get_register(oprs[i]);
        reg.port_main = oprs[i-1];
        reg.port_aux1 = terms[i+1];
        sim.rexu().set_register(oprs[i], reg);
    }

    // Collect wire info (oldest first)
    struct WireInfo { size_t opr; size_t target; };
    WireInfo wires[16];
    int wc = 0;

    wires[wc++] = {oprs[0], terms[0]};
    for (int i = 1; i < count - 1; i++) {
        wires[wc++] = {oprs[i], oprs[i-1]};
    }

    size_t result = count >= 2 ? oprs[count - 2] : terms[0];

    // Add wires in REVERSE order
    for (int i = wc - 1; i >= 0; i--) {
        sim.add_wire(wires[i].opr, 0, wires[i].target, 0);
    }

    return result;
}

int main() {
    printf("========================================\n");
    printf("Pure ICM Matrix-Vector Multiplication\n");
    printf("========================================\n\n");

    printf("Problem: y = A * x where:\n");
    printf("  A = [[2, 1], [3, 4]], x = [5, 6]\n");
    printf("  y[0] = 2*5 + 1*6 = 16\n");
    printf("  y[1] = 3*5 + 4*6 = 39\n\n");

    int A[MAT_SIZE][MAT_SIZE] = {{2, 1}, {3, 4}};
    int x[MAT_SIZE] = {5, 6};

    icm::ICMSimulator sim;

    printf("Building ICM network:\n");

    // Phase 1: Create input registers
    printf("  Phase 1: Creating inputs...\n");
    for (int i = 0; i < MAT_SIZE; i++) {
        for (int j = 0; j < MAT_SIZE; j++) {
            sim.rexu().create_num(icm::NumType::LIT, A[i][j]);
        }
    }
    for (int i = 0; i < MAT_SIZE; i++) {
        sim.rexu().create_num(icm::NumType::LIT, x[i]);
    }

    // Phase 2: Build multiplication chains ONE AT A TIME, run() after each
    printf("  Phase 2: Building mul chains (run after each)...\n");

    size_t prods[4];

    // Chain 1: 2*5 = 10
    printf("    Building prod[0] = %d*%d...\n", A[0][0], x[0]);
    prods[0] = create_mul_chain(sim, A[0][0], x[0]);
    printf("    prod[0] at reg %zu, running...\n", prods[0]);
    sim.run();
    printf("    After run: prod[0] = %u\n", sim.rexu().get_register(prods[0]).port_main);

    // Chain 2: 1*6 = 6
    printf("    Building prod[1] = %d*%d...\n", A[0][1], x[1]);
    prods[1] = create_mul_chain(sim, A[0][1], x[1]);
    printf("    prod[1] at reg %zu, running...\n", prods[1]);
    sim.run();
    printf("    After run: prod[1] = %u\n", sim.rexu().get_register(prods[1]).port_main);

    // Chain 3: 3*5 = 15
    printf("    Building prod[2] = %d*%d...\n", A[1][0], x[0]);
    prods[2] = create_mul_chain(sim, A[1][0], x[0]);
    printf("    prod[2] at reg %zu, running...\n", prods[2]);
    sim.run();
    printf("    After run: prod[2] = %u\n", sim.rexu().get_register(prods[2]).port_main);

    // Chain 4: 4*6 = 24
    printf("    Building prod[3] = %d*%d...\n", A[1][1], x[1]);
    prods[3] = create_mul_chain(sim, A[1][1], x[1]);
    printf("    prod[3] at reg %zu, running...\n", prods[3]);
    sim.run();
    printf("    After run: prod[3] = %u\n", sim.rexu().get_register(prods[3]).port_main);

    // Phase 3: Build sum chains (run after each)
    printf("  Phase 3: Building sum chains...\n");

    // Sum row 0
    printf("    Building y[0] = prod[0] + prod[1]...\n");
    size_t row0[2] = {prods[0], prods[1]};
    g_result_regs[0] = create_add_chain(sim, row0, 2, "y0");
    printf("    y[0] at reg %zu, running...\n", g_result_regs[0]);
    sim.run();
    printf("    After run: y[0] = %u\n", sim.rexu().get_register(g_result_regs[0]).port_main);

    // Sum row 1
    printf("    Building y[1] = prod[2] + prod[3]...\n");
    size_t row1[2] = {prods[2], prods[3]};
    g_result_regs[1] = create_add_chain(sim, row1, 2, "y1");
    printf("    y[1] at reg %zu, running...\n", g_result_regs[1]);
    sim.run();
    printf("    After run: y[1] = %u\n", sim.rexu().get_register(g_result_regs[1]).port_main);

    printf("\n--- Final Results ---\n");
    int y0 = sim.rexu().get_register(g_result_regs[0]).port_main;
    int y1 = sim.rexu().get_register(g_result_regs[1]).port_main;
    printf("  y[0] = %d (expected 16)\n", y0);
    printf("  y[1] = %d (expected 39)\n", y1);

    bool pass = (y0 == 16 && y1 == 39);
    printf("\n  Status: %s\n", pass ? "PASS" : "FAIL");

    return pass ? 0 : 1;
}