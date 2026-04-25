#include "../include/icm_core.hpp"
#include <cstdio>

int test_count = 0;
int pass_count = 0;

#define TEST(name) do { \
    printf("\n=== Test: %s ===\n", name); \
    test_count++; \
} while(0)

#define CHECK_ADDR(addr, expected_type, expected_val) do { \
    icm::RedexRegister reg = sim.rexu().get_register(addr); \
    icm::CombinatorType t = static_cast<icm::CombinatorType>(reg.metadata & 0xFFFF); \
    bool type_ok = (t == expected_type); \
    bool val_ok = (reg.port_main == expected_val); \
    if (type_ok && val_ok) { \
        printf("  PASS: [%zu] %s (val=%u)\n", addr, #expected_type, expected_val); \
        pass_count++; \
    } else { \
        printf("  FAIL: expected %s(val=%u), got ", #expected_type, expected_val); \
        switch(t) { \
            case icm::CombinatorType::CON: printf("CON"); break; \
            case icm::CombinatorType::DUP: printf("DUP"); break; \
            case icm::CombinatorType::ERA: printf("ERA"); break; \
            case icm::CombinatorType::VAR: printf("VAR"); break; \
            case icm::CombinatorType::NUM: printf("NUM(val=%u)", reg.port_main); break; \
            case icm::CombinatorType::OPR: printf("OPR"); break; \
            case icm::CombinatorType::SWI: printf("SWI"); break; \
            case icm::CombinatorType::FAN: printf("FAN"); break; \
            case icm::CombinatorType::BRA: printf("BRA"); break; \
            case icm::CombinatorType::VCON: printf("VCON"); break; \
            case icm::CombinatorType::IO: printf("IO"); break; \
            default: printf("UNKNOWN(0x%08X)", reg.metadata); break; \
        } \
        printf("\n"); \
    } \
} while(0)

#define CHECK_REDUCTIONS(expected) do { \
    if (sim.get_reduction_count() == expected) { \
        printf("  PASS: reductions=%d\n", expected); \
        pass_count++; \
    } else { \
        printf("  FAIL: expected reductions=%d, got %d\n", expected, sim.get_reduction_count()); \
    } \
} while(0)

void test_core_combinators() {
    printf("\n\n########## CORE COMBINATOR TESTS ##########\n");

    // CON ~ CON => ANNI (both become ERA)
    TEST("CON ~ CON => ANNI");
    {
        icm::ICMSimulator sim;
        size_t con_a = sim.rexu().create_con(0, 0, 0);
        size_t con_b = sim.rexu().create_con(0, 0, 0);
        sim.add_wire(con_a, 0, con_b, 0);
        sim.run();
        CHECK_ADDR(con_a, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(con_b, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // DUP ~ DUP => ANNI (both become ERA)
    TEST("DUP ~ DUP => ANNI");
    {
        icm::ICMSimulator sim;
        size_t dup_a = sim.rexu().create_dup(0, 0, 0);
        size_t dup_b = sim.rexu().create_dup(0, 0, 0);
        sim.add_wire(dup_a, 0, dup_b, 0);
        sim.run();
        CHECK_ADDR(dup_a, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(dup_b, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // ERA ~ ERA => VOID (no reduction, they coexist)
    TEST("ERA ~ ERA => VOID (coexist, no reduction)");
    {
        icm::ICMSimulator sim;
        size_t era_a = sim.rexu().create_era();
        size_t era_b = sim.rexu().create_era();
        sim.add_wire(era_a, 0, era_b, 0);
        sim.run();
        CHECK_ADDR(era_a, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(era_b, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(0);  // No reduction occurs
    }

    // ERA ~ NUM => ERAS (NUM becomes ERA)
    TEST("ERA ~ NUM => ERAS");
    {
        icm::ICMSimulator sim;
        size_t era = sim.rexu().create_era();
        size_t num = sim.rexu().create_num(icm::NumType::LIT, 42);
        sim.add_wire(era, 0, num, 0);
        sim.run();
        CHECK_ADDR(era, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(num, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // ERA ~ OPR => ERAS (OPR becomes ERA)
    TEST("ERA ~ OPR => ERAS");
    {
        icm::ICMSimulator sim;
        size_t num_a = sim.rexu().create_num(icm::NumType::LIT, 10);
        size_t num_b = sim.rexu().create_num(icm::NumType::LIT, 20);
        size_t opr = sim.rexu().create_opr(num_a, num_b, "+");
        size_t era = sim.rexu().create_era();
        sim.add_wire(era, 0, opr, 0);
        sim.run();
        CHECK_ADDR(era, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(opr, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // CON ~ ERA => ERAS (both become ERA)
    TEST("CON ~ ERA => ERAS");
    {
        icm::ICMSimulator sim;
        size_t con = sim.rexu().create_con(0, 0, 0);
        size_t era = sim.rexu().create_era();
        sim.add_wire(con, 0, era, 0);
        sim.run();
        CHECK_ADDR(con, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(era, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // DUP ~ ERA => ERAS (both become ERA)
    TEST("DUP ~ ERA => ERAS");
    {
        icm::ICMSimulator sim;
        size_t dup = sim.rexu().create_dup(0, 0, 0);
        size_t era = sim.rexu().create_era();
        sim.add_wire(dup, 0, era, 0);
        sim.run();
        CHECK_ADDR(dup, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(era, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // CON ~ DUP => COMM
    TEST("CON ~ DUP => COMM");
    {
        icm::ICMSimulator sim;
        size_t con = sim.rexu().create_con(0, 0, 0);
        size_t dup = sim.rexu().create_dup(0, 0, 0);
        sim.add_wire(con, 0, dup, 0);
        sim.run();
        CHECK_REDUCTIONS(1);  // COMM reduction occurred
    }
}

void test_numerical_combinators() {
    printf("\n\n########## NUMERICAL COMBINATOR TESTS ##########\n");

    // NUM ~ NUM => VOID (no reduction, they coexist)
    TEST("NUM ~ NUM => VOID (coexist, no reduction)");
    {
        icm::ICMSimulator sim;
        size_t num_a = sim.rexu().create_num(icm::NumType::LIT, 10);
        size_t num_b = sim.rexu().create_num(icm::NumType::LIT, 20);
        sim.add_wire(num_a, 0, num_b, 0);
        sim.run();
        CHECK_ADDR(num_a, icm::CombinatorType::NUM, 10);
        CHECK_ADDR(num_b, icm::CombinatorType::NUM, 20);
        CHECK_REDUCTIONS(0);  // No reduction
    }

    // OPR ~ OPR => ANNI (both become ERA)
    TEST("OPR ~ OPR => ANNI");
    {
        icm::ICMSimulator sim;
        size_t num_a1 = sim.rexu().create_num(icm::NumType::LIT, 1);
        size_t num_a2 = sim.rexu().create_num(icm::NumType::LIT, 2);
        size_t num_b1 = sim.rexu().create_num(icm::NumType::LIT, 3);
        size_t num_b2 = sim.rexu().create_num(icm::NumType::LIT, 4);
        size_t opr_a = sim.rexu().create_opr(num_a1, num_a2, "+");
        size_t opr_b = sim.rexu().create_opr(num_b1, num_b2, "+");
        sim.add_wire(opr_a, 0, opr_b, 0);
        sim.run();
        CHECK_ADDR(opr_a, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(opr_b, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // NUM ~ OPR => OPER: OPR(left=NUM(a), right=NUM(b)) reduces to NUM(a+b)
    // Wire: OPR.principal <-> NUM.principal
    TEST("NUM(3) ~ OPR(3, 5) => OPER (3+5=8)");
    {
        icm::ICMSimulator sim;
        size_t num_left = sim.rexu().create_num(icm::NumType::LIT, 3);
        size_t num_right = sim.rexu().create_num(icm::NumType::LIT, 5);
        size_t opr = sim.rexu().create_opr(num_left, num_right, "+");
        sim.add_wire(opr, 0, num_left, 0);
        sim.run();
        CHECK_ADDR(opr, icm::CombinatorType::NUM, 8);
        CHECK_REDUCTIONS(1);
    }

    // OPR ~ NUM: OPR(main=left, aux1=right) ~ NUM triggers OPER
    // Wire: OPR.principal <-> NUM.principal
    // OPR(10, 20) + NUM(10) => left_val(10) + right_val(20) = 30
    TEST("OPR(10, 20) ~ NUM(10) => OPER (10+20=30)");
    {
        icm::ICMSimulator sim;
        size_t num_a = sim.rexu().create_num(icm::NumType::LIT, 10);
        size_t num_b = sim.rexu().create_num(icm::NumType::LIT, 20);
        size_t num_ctrl = sim.rexu().create_num(icm::NumType::LIT, 10);
        size_t opr = sim.rexu().create_opr(num_a, num_b, "+");
        sim.add_wire(opr, 0, num_ctrl, 0);
        sim.run();
        CHECK_ADDR(opr, icm::CombinatorType::NUM, 30);
        CHECK_REDUCTIONS(1);
    }

    // OPR with ERA erases OPR
    TEST("OPR(5, 10) ~ ERA => ERAS");
    {
        icm::ICMSimulator sim;
        size_t num_a = sim.rexu().create_num(icm::NumType::LIT, 5);
        size_t num_b = sim.rexu().create_num(icm::NumType::LIT, 10);
        size_t opr = sim.rexu().create_opr(num_a, num_b, "+");
        size_t era = sim.rexu().create_era();
        sim.add_wire(opr, 0, era, 0);
        sim.run();
        CHECK_ADDR(opr, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }
}

void test_control_combinators() {
    printf("\n\n########## CONTROL COMBINATOR TESTS ##########\n");

    // SWI ~ SWI => ANNI
    TEST("SWI ~ SWI => ANNI");
    {
        icm::ICMSimulator sim;
        size_t swi_a = sim.rexu().create_combinator(icm::CombinatorType::SWI, 0, 0, 0);
        size_t swi_b = sim.rexu().create_combinator(icm::CombinatorType::SWI, 0, 0, 0);
        sim.add_wire(swi_a, 0, swi_b, 0);
        sim.run();
        CHECK_ADDR(swi_a, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(swi_b, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // SWI ~ OPR => COMM
    TEST("SWI ~ OPR => COMM");
    {
        icm::ICMSimulator sim;
        size_t num_a = sim.rexu().create_num(icm::NumType::LIT, 1);
        size_t num_b = sim.rexu().create_num(icm::NumType::LIT, 2);
        size_t opr = sim.rexu().create_opr(num_a, num_b, "+");
        size_t swi = sim.rexu().create_combinator(icm::CombinatorType::SWI, 0, 0, 0);
        sim.add_wire(swi, 0, opr, 0);
        sim.run();
        CHECK_REDUCTIONS(1);
    }

    // SWI ~ NUM => SWIT
    TEST("SWI ~ NUM => SWIT");
    {
        icm::ICMSimulator sim;
        size_t num = sim.rexu().create_num(icm::NumType::LIT, 5);
        size_t swi = sim.rexu().create_combinator(icm::CombinatorType::SWI, 0, 0, 0);
        sim.add_wire(swi, 0, num, 0);
        sim.run();
        CHECK_ADDR(swi, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(num, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }
}

void test_bookkeeping_combinators() {
    printf("\n\n########## BOOKKEEPING COMBINATOR TESTS ##########\n");

    // FAN ~ FAN => ANNI
    TEST("FAN ~ FAN => ANNI");
    {
        icm::ICMSimulator sim;
        size_t fan_a = sim.rexu().create_combinator(icm::CombinatorType::FAN, 0, 0, 0);
        size_t fan_b = sim.rexu().create_combinator(icm::CombinatorType::FAN, 0, 0, 0);
        sim.add_wire(fan_a, 0, fan_b, 0);
        sim.run();
        CHECK_ADDR(fan_a, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(fan_b, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }

    // BRA ~ BRA => ANNI
    TEST("BRA ~ BRA => ANNI");
    {
        icm::ICMSimulator sim;
        size_t bra_a = sim.rexu().create_combinator(icm::CombinatorType::BRA, 0, 0, 0);
        size_t bra_b = sim.rexu().create_combinator(icm::CombinatorType::BRA, 0, 0, 0);
        sim.add_wire(bra_a, 0, bra_b, 0);
        sim.run();
        CHECK_ADDR(bra_a, icm::CombinatorType::ERA, 0);
        CHECK_ADDR(bra_b, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }
}

void test_dense_combinators() {
    printf("\n\n########## DENSE COMBINATOR TESTS ##########\n");

    // VCON ~ VCON => VOID (no reduction, they coexist)
    TEST("VCON ~ VCON => VOID (coexist, no reduction)");
    {
        icm::ICMSimulator sim;
        size_t vcon_a = sim.rexu().create_combinator(icm::CombinatorType::VCON, 0, 0, 0);
        size_t vcon_b = sim.rexu().create_combinator(icm::CombinatorType::VCON, 0, 0, 0);
        sim.add_wire(vcon_a, 0, vcon_b, 0);
        sim.run();
        CHECK_ADDR(vcon_a, icm::CombinatorType::VCON, 0);
        CHECK_ADDR(vcon_b, icm::CombinatorType::VCON, 0);
        CHECK_REDUCTIONS(0);
    }

    // VCON ~ NUM => VOID (no reduction)
    TEST("VCON ~ NUM => VOID (coexist, no reduction)");
    {
        icm::ICMSimulator sim;
        size_t vcon = sim.rexu().create_combinator(icm::CombinatorType::VCON, 0, 0, 0);
        size_t num = sim.rexu().create_num(icm::NumType::LIT, 100);
        sim.add_wire(vcon, 0, num, 0);
        sim.run();
        CHECK_REDUCTIONS(0);
    }

    // VCON ~ ERA => ERAS
    TEST("VCON ~ ERA => ERAS");
    {
        icm::ICMSimulator sim;
        size_t vcon = sim.rexu().create_combinator(icm::CombinatorType::VCON, 0, 0, 0);
        size_t era = sim.rexu().create_era();
        sim.add_wire(vcon, 0, era, 0);
        sim.run();
        CHECK_ADDR(vcon, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }
}

void test_io_combinators() {
    printf("\n\n########## I/O COMBINATOR TESTS ##########\n");

    // IO ~ IO => PIPE (implementation-defined)
    TEST("IO ~ IO => PIPE");
    {
        icm::ICMSimulator sim;
        size_t io_a = sim.rexu().create_combinator(icm::CombinatorType::IO, 0, 0, 0);
        size_t io_b = sim.rexu().create_combinator(icm::CombinatorType::IO, 0, 0, 0);
        sim.add_wire(io_a, 0, io_b, 0);
        sim.run();
        printf("  (IO~IO result is implementation-defined)\n");
        CHECK_REDUCTIONS(0);  // IO~IO doesn't reduce in our implementation
    }

    // IO ~ ERA => ERAS
    TEST("IO ~ ERA => ERAS");
    {
        icm::ICMSimulator sim;
        size_t io = sim.rexu().create_combinator(icm::CombinatorType::IO, 0, 0, 0);
        size_t era = sim.rexu().create_era();
        sim.add_wire(io, 0, era, 0);
        sim.run();
        CHECK_ADDR(io, icm::CombinatorType::ERA, 0);
        CHECK_REDUCTIONS(1);
    }
}

int main() {
    printf("========================================\n");
    printf("ICM Simulator - Comprehensive Tests\n");
    printf("========================================\n");

    test_core_combinators();
    test_numerical_combinators();
    test_control_combinators();
    test_bookkeeping_combinators();
    test_dense_combinators();
    test_io_combinators();

    printf("\n\n========================================\n");
    printf("Test Results: %d/%d passed\n", pass_count, test_count);
    printf("========================================\n");

    return (pass_count == test_count) ? 0 : 1;
}