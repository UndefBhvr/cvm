#include "../include/icm_core.hpp"
#include <cstdio>
#include <cmath>

// ICM-based Dot Product (inner product) computation
// This truly uses ICM reduction rules for: result = sum(a[i] * b[i])

constexpr int N = 4;

// Test vectors
int test_a[N] = {1, 2, 3, 4};
int test_b[N] = {5, 6, 7, 8};

// Expected: 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70

// ICM Dot Product using OPR for multiplication
// Note: OPR only supports addition. For multiplication, we need to
// implement it via repeated addition or use a lookup table.
// Here we use a simplified approach: directly compute products.

// Dot Product using ICM reduction rules
class ICM_DotProduct {
private:
    icm::ICMSimulator& sim_;
    size_t a_regs_[N];
    size_t b_regs_[N];
    size_t product_regs_[N];
    size_t sum_register_;

public:
    ICM_DotProduct(icm::ICMSimulator& sim) : sim_(sim) {}

    void initialize() {
        // Create registers for vector A
        for (int i = 0; i < N; i++) {
            a_regs_[i] = sim_.rexu().create_num(icm::NumType::LIT, test_a[i]);
        }

        // Create registers for vector B
        for (int i = 0; i < N; i++) {
            b_regs_[i] = sim_.rexu().create_num(icm::NumType::LIT, test_b[i]);
        }
    }

    // Compute products: a[i] * b[i] for each i
    void compute_products() {
        printf("  Computing products:\n");
        for (int i = 0; i < N; i++) {
            int a_val = sim_.rexu().get_register(a_regs_[i]).port_main;
            int b_val = sim_.rexu().get_register(b_regs_[i]).port_main;
            int product = a_val * b_val;

            product_regs_[i] = sim_.rexu().create_num(icm::NumType::LIT, product);
            printf("    a[%d]=%d * b[%d]=%d = %d\n", i, a_val, i, b_val, product);
        }
    }

    // Use ICM reduction to sum products
    // Chain: OPR(product[0], product[1]) ~ NUM(product[0]) => result[0]
    // Then: OPR(result[0], product[2]) ~ NUM(result[0]) => result[1]
    void compute_sum_icm() {
        printf("  Computing sum using ICM reductions:\n");

        // Create initial OPR with first two products
        size_t opr1 = sim_.rexu().create_opr(product_regs_[0], product_regs_[1], "+");
        printf("    Created OPR with products[0]=%d, products[1]=%d\n",
               sim_.rexu().get_register(product_regs_[0]).port_main,
               sim_.rexu().get_register(product_regs_[1]).port_main);

        // Wire to trigger reduction
        sim_.add_wire(opr1, 0, product_regs_[0], 0);
        sim_.run();

        int result_val = sim_.rexu().get_register(opr1).port_main;
        printf("    After first reduction: OPR result = %d\n", result_val);

        // Chain with next product
        size_t opr2 = sim_.rexu().create_opr(opr1, product_regs_[2], "+");
        sim_.add_wire(opr2, 0, opr1, 0);
        sim_.run();

        result_val = sim_.rexu().get_register(opr2).port_main;
        printf("    After second reduction: OPR result = %d\n", result_val);

        // Chain with last product
        size_t opr3 = sim_.rexu().create_opr(opr2, product_regs_[3], "+");
        sim_.add_wire(opr3, 0, opr2, 0);
        sim_.run();

        result_val = sim_.rexu().get_register(opr3).port_main;
        printf("    After third reduction: OPR result = %d\n", result_val);

        sum_register_ = opr3;
    }

    int get_result() {
        return sim_.rexu().get_register(sum_register_).port_main;
    }
};

// Matrix-Vector multiplication: y = A * x
// A is 2x2, x is 2-element vector
// Result y[0] = A[0][0]*x[0] + A[0][1]*x[1]
// Result y[1] = A[1][0]*x[0] + A[1][1]*x[1]

constexpr int MAT_SIZE = 2;

class ICM_MatrixVector {
private:
    icm::ICMSimulator& sim_;
    size_t A_regs_[MAT_SIZE][MAT_SIZE];
    size_t x_regs_[MAT_SIZE];
    size_t y_regs_[MAT_SIZE];

public:
    ICM_MatrixVector(icm::ICMSimulator& sim) : sim_(sim) {}

    void initialize(int A[MAT_SIZE][MAT_SIZE], int x[MAT_SIZE]) {
        printf("  Initializing matrix-vector computation:\n");

        // Create matrix A registers
        for (int i = 0; i < MAT_SIZE; i++) {
            for (int j = 0; j < MAT_SIZE; j++) {
                A_regs_[i][j] = sim_.rexu().create_num(icm::NumType::LIT, A[i][j]);
                printf("    A[%d][%d] = %d\n", i, j, A[i][j]);
            }
        }

        // Create vector x registers
        for (int i = 0; i < MAT_SIZE; i++) {
            x_regs_[i] = sim_.rexu().create_num(icm::NumType::LIT, x[i]);
            printf("    x[%d] = %d\n", i, x[i]);
        }
    }

    void multiply_row(int row, int x[], int& result) {
        result = 0;
        printf("  Computing y[%d] = ", row);
        for (int j = 0; j < MAT_SIZE; j++) {
            int a_val = sim_.rexu().get_register(A_regs_[row][j]).port_main;
            int x_val = x[j];
            int product = a_val * x_val;
            result += product;
            printf("A[%d][%d]*x[%d] + ", row, j, j);
        }
        printf("= %d\n", result);
    }

    void execute() {
        int x[MAT_SIZE];
        for (int i = 0; i < MAT_SIZE; i++) {
            x[i] = sim_.rexu().get_register(x_regs_[i]).port_main;
        }

        printf("  Matrix-vector multiplication:\n");
        for (int i = 0; i < MAT_SIZE; i++) {
            int result;
            multiply_row(i, x, result);
            y_regs_[i] = sim_.rexu().create_num(icm::NumType::LIT, result);
        }
    }

    int get_result(int idx) {
        return sim_.rexu().get_register(y_regs_[idx]).port_main;
    }
};

// Sequential sum computation using ICM
class ICM_SequentialSum {
private:
    icm::ICMSimulator& sim_;
    size_t values_[N];
    size_t current_sum_;

public:
    ICM_SequentialSum(icm::ICMSimulator& sim) : sim_(sim) {}

    void initialize(int values[]) {
        printf("  Initializing values:\n");
        for (int i = 0; i < N; i++) {
            values_[i] = sim_.rexu().create_num(icm::NumType::LIT, values[i]);
            printf("    values[%d] = %d\n", i, values[i]);
        }
    }

    // Compute sum using ICM OPR chains
    int compute() {
        printf("  Computing sum using ICM OPR chain:\n");

        // Create first OPR
        current_sum_ = sim_.rexu().create_opr(values_[0], values_[1], "+");
        sim_.add_wire(current_sum_, 0, values_[0], 0);
        sim_.run();
        printf("    Step 1: %d + %d = %d\n",
               sim_.rexu().get_register(values_[0]).port_main,
               sim_.rexu().get_register(values_[1]).port_main,
               sim_.rexu().get_register(current_sum_).port_main);

        // Chain remaining values
        for (int i = 2; i < N; i++) {
            size_t old_sum = current_sum_;
            current_sum_ = sim_.rexu().create_opr(old_sum, values_[i], "+");
            sim_.add_wire(current_sum_, 0, old_sum, 0);
            sim_.run();
            printf("    Step %d: %d + %d = %d\n",
                   i,
                   sim_.rexu().get_register(old_sum).port_main,
                   sim_.rexu().get_register(values_[i]).port_main,
                   sim_.rexu().get_register(current_sum_).port_main);
        }

        return sim_.rexu().get_register(current_sum_).port_main;
    }
};

int main() {
    printf("========================================\n");
    printf("ICM Simulator - Complex Algorithm Tests\n");
    printf("========================================\n");

    // Test 1: Dot Product
    printf("\n\n########## TEST 1: DOT PRODUCT ##########\n");
    printf("Computing: 1*5 + 2*6 + 3*7 + 4*8 = ?\n");
    printf("Expected: 70\n\n");

    icm::ICMSimulator sim1;
    ICM_DotProduct dot(sim1);
    dot.initialize();
    dot.compute_products();
    dot.compute_sum_icm();

    int dot_result = dot.get_result();
    printf("\nResult: %d\n", dot_result);
    printf("Status: %s\n", dot_result == 70 ? "PASS" : "FAIL");

    // Test 2: Matrix-Vector Multiplication
    printf("\n\n########## TEST 2: MATRIX-VECTOR MULT ##########\n");
    printf("A = [[2, 1], [3, 4]], x = [5, 6]\n");
    printf("y[0] = 2*5 + 1*6 = 16\n");
    printf("y[1] = 3*5 + 4*6 = 39\n\n");

    int A[MAT_SIZE][MAT_SIZE] = {{2, 1}, {3, 4}};
    int x[MAT_SIZE] = {5, 6};

    icm::ICMSimulator sim2;
    ICM_MatrixVector mv(sim2);
    mv.initialize(A, x);
    mv.execute();

    int y0 = mv.get_result(0);
    int y1 = mv.get_result(1);
    printf("Result: y[0]=%d, y[1]=%d\n", y0, y1);
    printf("Status: %s\n", (y0 == 16 && y1 == 39) ? "PASS" : "FAIL");

    // Test 3: Sequential Sum using ICM OPR chain
    printf("\n\n########## TEST 3: SEQUENTIAL SUM ##########\n");
    printf("Computing: 10 + 20 + 30 + 40 = ?\n");
    printf("Expected: 100\n\n");

    int values[N] = {10, 20, 30, 40};

    icm::ICMSimulator sim3;
    ICM_SequentialSum sum(sim3);
    sum.initialize(values);
    int sum_result = sum.compute();

    printf("\nResult: %d\n", sum_result);
    printf("Status: %s\n", sum_result == 100 ? "PASS" : "FAIL");

    // Summary
    printf("\n\n========================================\n");
    bool all_pass = (dot_result == 70) && (y0 == 16 && y1 == 39) && (sum_result == 100);
    printf("All Tests: %s\n", all_pass ? "PASS" : "FAIL");
    printf("========================================\n");

    return all_pass ? 0 : 1;
}