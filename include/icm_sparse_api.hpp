#ifndef ICM_SPARSE_API_HPP
#define ICM_SPARSE_API_HPP

#include "icm_core.hpp"
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <memory>

namespace icm {

// Error codes
enum class Error {
    SUCCESS = 0,
    OUT_OF_MEMORY,
    INVALID_ARGUMENT,
    DEVICE_ERROR,
    KERNEL_FAILED
};

// Forward declarations
class Context;
template<typename T> class SparseMatrixCSR;
template<typename T> class DenseMatrix;

// Context - represents the ICM "device"
class Context {
public:
    Context() : simulator_(std::make_unique<ICMSimulator>()), name_("ICM Device 0") {}

    ICMSimulator* simulator() { return simulator_.get(); }
    const char* name() const { return name_; }

private:
    std::unique_ptr<ICMSimulator> simulator_;
    const char* name_;
};

// Dense Matrix - stored in row-major format
template<typename T>
class DenseMatrix {
public:
    DenseMatrix(Context* ctx, int rows, int cols)
        : context_(ctx), rows_(rows), cols_(cols), nnz_(rows * cols) {
        data_.resize(rows * cols, 0);
    }

    DenseMatrix(Context* ctx) : context_(ctx), rows_(0), cols_(0), nnz_(0) {}

    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int nnz() const { return nnz_; }
    Context* context() const { return context_; }

    void set_data(const std::vector<T>& src) {
        data_ = src;
    }

    T get(int row, int col) const {
        return data_[row * cols_ + col];
    }

    void set(int row, int col, T val) {
        data_[row * cols_ + col] = val;
    }

    void print(const char* name = "DenseMatrix") const {
        printf("%s (%dx%d):\n", name, rows_, cols_);
        for (int i = 0; i < rows_; i++) {
            printf("  [");
            for (int j = 0; j < cols_; j++) {
                printf(" %d", (int)data_[i * cols_ + j]);
                if (j < cols_ - 1) printf(",");
            }
            printf(" ]\n");
        }
    }

private:
    Context* context_;
    int rows_;
    int cols_;
    int nnz_;
    std::vector<T> data_;
};

// Sparse Matrix in CSR format (Compressed Sparse Row)
template<typename T>
class SparseMatrixCSR {
public:
    SparseMatrixCSR(Context* ctx)
        : context_(ctx), rows_(0), cols_(0), nnz_(0) {}

    SparseMatrixCSR(Context* ctx, int rows, int cols, int nnz)
        : context_(ctx), rows_(rows), cols_(cols), nnz_(nnz) {
        values_.resize(nnz);
        col_idx_.resize(nnz);
        row_ptr_.resize(rows + 1);
    }

    // Initialize from host data (CSR format)
    Error initialize(const T* h_values, const int* h_col_idx,
                     const int* h_row_ptr, int nnz, int rows, int cols) {
        if (!h_values || !h_col_idx || !h_row_ptr) {
            return Error::INVALID_ARGUMENT;
        }

        this->nnz_ = nnz;
        this->rows_ = rows;
        this->cols_ = cols;

        values_.resize(nnz);
        col_idx_.resize(nnz);
        row_ptr_.resize(rows + 1);

        std::memcpy(values_.data(), h_values, nnz * sizeof(T));
        std::memcpy(col_idx_.data(), h_col_idx, nnz * sizeof(int));
        std::memcpy(row_ptr_.data(), h_row_ptr, (rows + 1) * sizeof(int));

        return Error::SUCCESS;
    }

    T* values() { return values_.data(); }
    int* col_idx() { return col_idx_.data(); }
    int* row_ptr() { return row_ptr_.data(); }
    const T* values() const { return values_.data(); }
    const int* col_idx() const { return col_idx_.data(); }
    const int* row_ptr() const { return row_ptr_.data(); }

    int nnz() const { return nnz_; }
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    Context* context() const { return context_; }

    void print(const char* name = "SparseMatrixCSR") const {
        printf("%s (%dx%d, nnz=%d):\n", name, rows_, cols_, nnz_);
        printf("  values: ");
        for (int i = 0; i < nnz_; i++) printf("%d ", (int)values_[i]);
        printf("\n");
        printf("  col_idx: ");
        for (int i = 0; i < nnz_; i++) printf("%d ", col_idx_[i]);
        printf("\n");
        printf("  row_ptr: ");
        for (int i = 0; i <= rows_; i++) printf("%d ", row_ptr_[i]);
        printf("\n");
    }

    // Get non-zero at linear index i
    T get_value(int i) const { return values_[i]; }
    int get_col(int i) const { return col_idx_[i]; }
    int get_row_start(int row) const { return row_ptr_[row]; }
    int get_row_end(int row) const { return row_ptr_[row + 1]; }

private:
    Context* context_;
    int rows_;
    int cols_;
    int nnz_;
    std::vector<T> values_;
    std::vector<int> col_idx_;
    std::vector<int> row_ptr_;
};

// API Functions

// Create/destroy context
Context* create_context();
void destroy_context(Context* ctx);

// Create sparse matrix from CSR format
template<typename T>
SparseMatrixCSR<T>* create_sparse_matrix(Context* ctx,
                                        const T* h_values,
                                        const int* h_col_idx,
                                        const int* h_row_ptr,
                                        int nnz, int rows, int cols) {
    SparseMatrixCSR<T>* mat = new SparseMatrixCSR<T>(ctx, rows, cols, nnz);
    Error err = mat->initialize(h_values, h_col_idx, h_row_ptr, nnz, rows, cols);
    if (err != Error::SUCCESS) {
        delete mat;
        return nullptr;
    }
    return mat;
}

// Create dense matrix from data
template<typename T>
DenseMatrix<T>* create_dense_matrix(Context* ctx, const T* h_data, int rows, int cols) {
    DenseMatrix<T>* mat = new DenseMatrix<T>(ctx, rows, cols);
    for (int i = 0; i < rows * cols; i++) {
        mat->data()[i] = h_data[i];
    }
    return mat;
}

// Destroy matrices
template<typename T>
void destroy_sparse_matrix(SparseMatrixCSR<T>* mat) {
    delete mat;
}

template<typename T>
void destroy_dense_matrix(DenseMatrix<T>* mat) {
    delete mat;
}

// Build ICM multiplication chain for a * b
// Returns register address containing result
inline size_t build_icm_mul_chain(ICMSimulator& sim, int a_val, int b_val) {
    if (b_val <= 0) {
        return sim.rexu().create_num(icm::NumType::LIT, 0);
    }
    if (b_val == 1) {
        return sim.rexu().create_num(icm::NumType::LIT, (icm::XLEN_t)a_val);
    }

    // Create b_val copies of a_val
    std::vector<size_t> nums;
    for (int i = 0; i < b_val; i++) {
        nums.push_back(sim.rexu().create_num(icm::NumType::LIT, (icm::XLEN_t)a_val));
    }

    // Build chain: OPR(current, nums[i]) for i = 1 to b_val-1
    size_t current = nums[0];

    struct WireInfo { size_t opr; size_t target; };
    std::vector<WireInfo> wires;

    for (int i = 1; i < b_val; i++) {
        size_t opr = sim.rexu().create_opr(current, nums[i], "+");
        wires.push_back({opr, current});
        current = opr;
    }

    // Add wires in reverse order so oldest is found first
    for (int i = (int)wires.size() - 1; i >= 0; i--) {
        sim.add_wire(wires[i].opr, 0, wires[i].target, 0);
    }

    return current;
}

// Build ICM addition chain for summing multiple values
inline size_t build_icm_add_chain(ICMSimulator& sim, const std::vector<size_t>& terms) {
    if (terms.empty()) {
        return sim.rexu().create_num(icm::NumType::LIT, 0);
    }
    if (terms.size() == 1) {
        return terms[0];
    }

    size_t result = terms[0];
    for (size_t i = 1; i < terms.size(); i++) {
        size_t sum_opr = sim.rexu().create_opr(result, terms[i], "+");
        sim.add_wire(sum_opr, 0, result, 0);
        result = sum_opr;
    }

    return result;
}

// Sparse Matrix * Dense Matrix -> Dense Matrix
// C = A * B, where A is sparse (CSR), B is dense
// All computation done via ICM reduction rules
template<typename T>
Error spmm(Context* ctx, const SparseMatrixCSR<T>* A,
           const DenseMatrix<T>* B, DenseMatrix<T>* C) {
    if (!ctx || !A || !B || !C) {
        return Error::INVALID_ARGUMENT;
    }

    if (A->cols() != B->rows()) {
        return Error::INVALID_ARGUMENT;
    }

    int m = A->rows();    // output rows
    int n = B->cols();    // output cols
    int k = A->cols();    // inner dimension

    ICMSimulator& sim = *ctx->simulator();

    // Create result matrix
    C = new DenseMatrix<T>(ctx, m, n);

    // Result registers: one per output element
    std::vector<size_t> result_regs(m * n, 0);
    std::vector<char> valid(m * n, 0);

    // For each output element C[i][j]
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            // Collect products for C[i][j] = sum_p A[i][p] * B[p][j]
            std::vector<size_t> prod_regs;

            int row_start = A->get_row_start(i);
            int row_end = A->get_row_end(i);

            for (int idx = row_start; idx < row_end; idx++) {
                T a_val = A->get_value(idx);
                int p = A->get_col(idx);  // A[i][p]

                T b_val = B->get(p, j);  // B[p][j]

                // Build multiplication chain for a_val * b_val via ICM
                size_t prod_reg = build_icm_mul_chain(sim, (int)a_val, (int)b_val);

                // Run the multiplication chain
                sim.run();

                // Store the result register
                prod_regs.push_back(prod_reg);
            }

            // Sum all products via ICM
            if (!prod_regs.empty()) {
                size_t sum_reg = build_icm_add_chain(sim, prod_regs);
                sim.run();
                result_regs[i * n + j] = sum_reg;
                valid[i * n + j] = 1;
            } else {
                // Zero result
                result_regs[i * n + j] = sim.rexu().create_num(icm::NumType::LIT, 0);
            }
        }
    }

    // Read results from ICM registers
    sim.run();
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            int idx = i * n + j;
            if (valid[idx]) {
                icm::XLEN_t val = sim.rexu().get_register(result_regs[idx]).port_main;
                C->set(i, j, (T)val);
            }
        }
    }

    return Error::SUCCESS;
}

// Sparse Matrix * Sparse Matrix -> Sparse Matrix
// C = A * B, all sparse in CSR format
template<typename T>
Error spmm(Context* ctx, const SparseMatrixCSR<T>* A,
           const SparseMatrixCSR<T>* B, SparseMatrixCSR<T>*& C) {
    if (!ctx || !A || !B) {
        return Error::INVALID_ARGUMENT;
    }

    if (A->cols() != B->rows()) {
        return Error::INVALID_ARGUMENT;
    }

    int m = A->rows();
    int n = B->cols();

    ICMSimulator& sim = *ctx->simulator();

    // Result entries: (row, col, value)
    struct CEntry {
        int row;
        int col;
        T value;
        size_t icm_reg;  // ICM register for this entry
    };
    std::vector<CEntry> c_entries;

    // For each row i of A
    for (int i = 0; i < m; i++) {
        // Map: col -> (icm_reg, count)
        // We accumulate sums using ICM
        std::vector<std::pair<int, std::vector<size_t>>> col_products;

        int row_start = A->get_row_start(i);
        int row_end = A->get_row_end(i);

        for (int idx = row_start; idx < row_end; idx++) {
            T a_val = A->get_value(idx);
            int p = A->get_col(idx);  // A[i][p]

            // Multiply row p of B by a_val
            int b_row_start = B->get_row_start(p);
            int b_row_end = B->get_row_end(p);

            for (int b_idx = b_row_start; b_idx < b_row_end; b_idx++) {
                T b_val = B->get_value(b_idx);
                int j = B->get_col(b_idx);  // B[p][j]

                // Compute a_val * b_val via ICM
                size_t prod_reg = build_icm_mul_chain(sim, (int)a_val, (int)b_val);
                sim.run();

                // Find or create entry for column j
                bool found = false;
                for (auto& cp : col_products) {
                    if (cp.first == j) {
                        cp.second.push_back(prod_reg);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    col_products.push_back({j, {prod_reg}});
                }
            }
        }

        // Sum products for each column using ICM
        for (auto& cp : col_products) {
            size_t sum_reg = build_icm_add_chain(sim, cp.second);
            sim.run();

            icm::XLEN_t val = sim.rexu().get_register(sum_reg).port_main;
            if (val != 0) {
                c_entries.push_back({i, cp.first, (T)val, sum_reg});
            }
        }
    }

    // Build C from entries
    int c_nnz = (int)c_entries.size();
    C = new SparseMatrixCSR<T>(ctx, m, n, c_nnz);

    for (int i = 0; i < c_nnz; i++) {
        C->values()[i] = c_entries[i].value;
        C->col_idx()[i] = c_entries[i].col;
    }

    // Build row_ptr
    std::vector<int> c_row_ptr(m + 1, 0);
    for (int i = 0; i < c_nnz; i++) {
        c_row_ptr[c_entries[i].row + 1]++;
    }
    for (int i = 0; i < m; i++) {
        c_row_ptr[i + 1] += c_row_ptr[i];
    }
    std::memcpy(C->row_ptr(), c_row_ptr.data(), (m + 1) * sizeof(int));

    return Error::SUCCESS;
}

} // namespace icm

#endif // ICM_SPARSE_API_HPP