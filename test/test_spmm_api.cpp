#include "../include/icm_sparse_api.hpp"
#include <cstdio>

// Test 4x4 sparse matrix * sparse matrix multiplication
//
// A = [[1, 0, 2, 0],
//      [0, 0, 3, 0],
//      [4, 0, 0, 5],
//      [0, 6, 0, 0]]
//
// B = [[1, 0, 0, 2],
//      [0, 0, 3, 0],
//      [2, 0, 0, 0],
//      [0, 0, 0, 1]]
//
// Manual calculation of C = A * B:
// Row 0: C[0][j] = A[0][0]*B[0][j] + A[0][2]*B[2][j]
//   C[0][0] = 1*1 + 2*2 = 1 + 4 = 5
//   C[0][3] = 1*2 + 2*0 = 2 + 0 = 2
//
// Row 1: C[1][j] = A[1][2]*B[2][j]
//   C[1][0] = 3*2 = 6
//   C[1][3] = 3*0 = 0
//
// Row 2: C[2][j] = A[2][0]*B[0][j] + A[2][3]*B[3][j]
//   C[2][0] = 4*1 + 5*0 = 4 + 0 = 4
//   C[2][3] = 4*2 + 5*1 = 8 + 5 = 13
//
// Row 3: C[3][j] = A[3][1]*B[1][j]
//   C[3][1] = 6*0 = 0
//   C[3][2] = 6*3 = 18

int main() {
    using namespace icm;

    printf("========================================\n");
    printf("ICM Sparse API - 4x4 Matrix * Matrix Test\n");
    printf("========================================\n\n");

    Context* ctx = create_context();
    printf("Created context: %s\n\n", ctx->name());

    // Matrix A (4x4) in CSR format
    int A_values[] = {1, 2, 3, 4, 5, 6};
    int A_col_idx[] = {0, 2, 2, 0, 3, 1};
    int A_row_ptr[] = {0, 2, 3, 5, 6};
    int A_nnz = 6;

    printf("Matrix A (4x4, nnz=%d):\n", A_nnz);
    printf("  values: ");
    for (int i = 0; i < A_nnz; i++) printf("%d ", A_values[i]);
    printf("\n  col_idx: ");
    for (int i = 0; i < A_nnz; i++) printf("%d ", A_col_idx[i]);
    printf("\n  row_ptr: ");
    for (int i = 0; i <= 4; i++) printf("%d ", A_row_ptr[i]);
    printf("\n\n");

    // Matrix B (4x4) in CSR format
    int B_values[] = {1, 2, 3, 2, 1};
    int B_col_idx[] = {0, 3, 2, 0, 3};
    int B_row_ptr[] = {0, 2, 3, 4, 5};
    int B_nnz = 5;

    printf("Matrix B (4x4, nnz=%d):\n", B_nnz);
    printf("  values: ");
    for (int i = 0; i < B_nnz; i++) printf("%d ", B_values[i]);
    printf("\n  col_idx: ");
    for (int i = 0; i < B_nnz; i++) printf("%d ", B_col_idx[i]);
    printf("\n  row_ptr: ");
    for (int i = 0; i <= 4; i++) printf("%d ", B_row_ptr[i]);
    printf("\n\n");

    SparseMatrixCSR<int>* A = create_sparse_matrix<int>(ctx, A_values, A_col_idx, A_row_ptr, A_nnz, 4, 4);
    SparseMatrixCSR<int>* B = create_sparse_matrix<int>(ctx, B_values, B_col_idx, B_row_ptr, B_nnz, 4, 4);

    if (!A || !B) {
        printf("ERROR: Failed to create matrices\n");
        destroy_context(ctx);
        return 1;
    }

    printf("Computing C = A * B via ICM...\n\n");

    SparseMatrixCSR<int>* C = nullptr;
    Error err = spmm(ctx, A, B, C);

    if (err != Error::SUCCESS) {
        printf("ERROR: spmm failed with error code %d\n", (int)err);
        destroy_sparse_matrix(A);
        destroy_sparse_matrix(B);
        destroy_context(ctx);
        return 1;
    }

    printf("Result C = A * B:\n");
    C->print("C");

    // Build dense view
    printf("\nDense view of C:\n");
    int C_dense[4][4] = {0};
    for (int i = 0; i < C->nnz(); i++) {
        int row = 0;
        for (int r = 0; r < 4; r++) {
            if (i >= C->row_ptr()[r] && i < C->row_ptr()[r + 1]) {
                row = r;
                break;
            }
        }
        int col = C->col_idx()[i];
        int val = C->values()[i];
        C_dense[row][col] = val;
    }

    for (int i = 0; i < 4; i++) {
        printf("  [");
        for (int j = 0; j < 4; j++) {
            printf(" %d", C_dense[i][j]);
            if (j < 3) printf(",");
        }
        printf(" ]\n");
    }

    printf("\nExpected C:\n");
    printf("  [ 5, 0, 0, 2 ]\n");
    printf("  [ 6, 0, 0, 0 ]\n");
    printf("  [ 4, 0, 0, 13]\n");
    printf("  [ 0, 0, 18, 0]\n");

    // Verify each entry
    bool pass = true;

    int checks[][3] = {
        {0, 0, 5},  // C[0][0] = 5
        {0, 3, 2},  // C[0][3] = 2
        {1, 0, 6},  // C[1][0] = 6
        {1, 3, 0},  // C[1][3] = 0
        {2, 0, 4},  // C[2][0] = 4
        {2, 3, 13}, // C[2][3] = 13
        {3, 1, 0},  // C[3][1] = 0
        {3, 2, 18}, // C[3][2] = 18
    };

    printf("\nVerification:\n");
    for (auto& check : checks) {
        int row = check[0];
        int col = check[1];
        int expected = check[2];
        int actual = C_dense[row][col];
        bool ok = (actual == expected);
        printf("  C[%d][%d] = %d (expected %d) - %s\n",
               row, col, actual, expected, ok ? "OK" : "FAIL");
        if (!ok) pass = false;
    }

    printf("\n  Status: %s\n", pass ? "PASS" : "FAIL");

    destroy_sparse_matrix(A);
    destroy_sparse_matrix(B);
    destroy_sparse_matrix(C);
    destroy_context(ctx);

    return pass ? 0 : 1;
}