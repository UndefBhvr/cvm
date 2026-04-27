# CVM: ICM (Interaction Combinator Machine) Software Simulator

CVM is a software emulator implementing the ICM architecture based on **Interaction Combinators**. It provides both a low-level simulator for testing reduction rules and a CUDA-like high-level API for sparse matrix operations.

## ICM Architecture Overview

ICM (Interaction Combinator Machine) is a graph-reduction computing architecture where:

- **Registers** hold combinators (NUM, OPR, CON, DUP, ERA, etc.)
- **Wires** connect ports between registers
- **Redexes** (reducible expressions) trigger reductions when compatible combinators interact
- **Reduction rules** define how combinator pairs transform:
  - `NUM ~ OPR` → `NUM` (arithmetic operation)
  - `ERA ~ X` → `ERA` (erasure)
  - `OPR ~ OPR` → `ERA` (annihilation)
  - And more...

## Project Structure

```
cvm/
├── include/
│   ├── icm_core.hpp         # Core ICM simulator (ICMSimulator, RedexExecutionUnit)
│   └── icm_sparse_api.hpp   # CUDA-like sparse matrix API
├── src/
│   ├── icm_core.cpp         # Core implementation (reduction rules)
│   └── icm_sparse_api.cpp   # API implementation
└── test/
    ├── Makefile
    ├── test_matrix.cpp      # Dense matrix-vector multiplication (pure ICM)
    ├── test_sparse.cpp      # Sparse matrix-vector multiplication (pure ICM)
    └── test_spmm_api.cpp    # Sparse matrix-matrix via API
```

## Building

```bash
cd cvm/test
make [target]    # Build specific test (e.g., make test_matrix)
make all         # Build all tests
make test         # Run matrix, sparse, and spmm_api tests
```

## Core Concepts

### Combinator Types

| Type | Description | Reduction Behavior |
|------|-------------|-------------------|
| `NUM` | Literal number value | NUM ~ NUM: coexist; NUM ~ OPR: operate |
| `OPR` | Binary operator (+, -, etc.) | OPR ~ NUM: compute; OPR ~ OPR: annihilate |
| `ERA` | Eraser | ERA ~ X: X becomes ERA |
| `CON` | Constructor | CON ~ CON: annihilate |
| `DUP` | Duplicator | DUP ~ DUP: annihilate |
| `SWI` | Switch | SWI ~ NUM: switch operation |

### Reduction Engine

The `ICMSimulator` maintains:
- **Registers**: array of `RedexRegister` holding combinators
- **Active wires**: connections between ports triggering redexes
- **Reduction count**: number of reductions performed

The `find_redex()` function scans wires to find valid redex pairs, and `step()` applies the appropriate reduction rule.

## Key Implementation Details

### Wire Ordering

**Critical**: Wire addition order matters! The reduction engine scans wires from newest to oldest (end of vector to beginning). For correct chain evaluation:

```cpp
// Add wires in REVERSE order so oldest is found first
for (int i = wc - 1; i >= 0; i--) {
    sim.add_wire(wires[i].opr, 0, wires[i].target, 0);
}
```

### Multiplication via Repeated Addition

ICM only supports addition through `NUM ~ OPR` reduction. Multiplication is implemented as repeated addition:

```
a * b = a + a + ... + a (b times)
```

### Running Reductions

Each `sim.run()` performs all possible reductions until no more redexes exist. For complex chains, run multiple times:

```cpp
sim.run();  // First batch of reductions
sim.run();  // Continue if needed
```

## API Reference

### Context

```cpp
Context* ctx = create_context();
ICMSimulator* sim = ctx->simulator();
// ... use context ...
destroy_context(ctx);
```

### Sparse Matrix (CSR Format)

```cpp
// Create from CSR format
SparseMatrixCSR<int>* A = create_sparse_matrix<int>(
    ctx,
    values,     // non-zero values
    col_idx,    // column indices
    row_ptr,    // row pointers
    nnz,        // number of non-zeros
    rows, cols  // matrix dimensions
);
```

### Sparse Matrix Multiplication (SpMM)

```cpp
// Sparse × Sparse → Sparse
SparseMatrixCSR<int>* C = nullptr;
spmm(ctx, A, B, C);  // C = A × B

// Sparse × Dense → Dense
DenseMatrix<int>* D = nullptr;
spmm(ctx, A, B, D);   // D = A × B
```

## Examples

### Simple Addition: 3 + 5

```cpp
ICMSimulator sim;
size_t num3 = sim.rexu().create_num(NumType::LIT, 3);
size_t num5 = sim.rexu().create_num(NumType::LIT, 5);
size_t opr = sim.rexu().create_opr(num3, num5, "+");
sim.add_wire(opr, 0, num3, 0);  // Wire triggers NUM ~ OPR reduction
sim.run();
// Result: opr register now holds NUM(8)
```

### Sparse Matrix-Vector Multiplication

```cpp
// A = [[1,0,2],[0,0,3],[4,0,0]], x = [1,2,3]
// y = A*x: y[0]=7, y[1]=9, y[2]=4

// Build ICM network, then:
sim.run();
int y0 = sim.rexu().get_register(result_y0).port_main;  // 7
```

## Testing

| Test | Description | Status |
|------|-------------|--------|
| `test_matrix` | Dense 2×2 matrix × vector | PASS |
| `test_sparse` | 4×4 sparse matrix × vector | PASS |
| `test_spmm_api` | 4×4 sparse matrix × matrix via API | PASS |

## Requirements

- C++17 compatible compiler (GCC 9+, Clang 10+)
- GNU Make

## References

- ICM architecture based on **Interaction Combinators** (Lafont, 1997)
- Designed for sparse matrix multiplication acceleration
- All computation in tests uses pure ICM reduction rules; C++ only handles initialization and result reading
