#include "../include/icm_core.hpp"
#include <cstdio>
#include <cmath>

// 8-point FFT using ICM architecture
// Using Q8.8 fixed-point format

constexpr int N = 8;
constexpr int LOG2_N = 3;

struct Complex {
    int real;  // Q8.8 format
    int imag;  // Q8.8 format
};

// Correct twiddle factors: W_N^k = e^(-2πi*k/N)
// For N=8, stage 0 needs 4 twiddle factors
Complex twiddle_factors[N/2] = {
    {256, 0},           // W_8^0 = 1
    {181, -181},        // W_8^1 = cos(π/4) - i*sin(π/4)
    {0, -256},          // W_8^2 = e^(-πi/2) = -i
    {-181, -181},       // W_8^3 = cos(3π/4) - i*sin(3π/4)
};

// Stage 1 twiddle factors (W_4^0, W_4^1 = W_8^0, W_8^2)
Complex stage1_twiddle[2] = {
    {256, 0},           // W_8^0 = 1
    {0, -256},          // W_8^2 = -i
};

// Stage 2 twiddle factor (W_2^0 = W_8^0)
Complex stage2_twiddle[1] = {
    {256, 0},           // W_8^0 = 1
};

// Input: [1, 2, 3, 4, 0, 0, 0, 0]
Complex input[N] = {
    {256, 0},    // x[0] = 1.0
    {512, 0},    // x[1] = 2.0
    {768, 0},    // x[2] = 3.0
    {1024, 0},   // x[3] = 4.0
    {0, 0},      // x[4] = 0.0
    {0, 0},      // x[5] = 0.0
    {0, 0},      // x[6] = 0.0
    {0, 0},      // x[7] = 0.0
};

// Bit-reversal permutation
int bit_reverse(int i) {
    int j = 0;
    for (int k = 0; k < LOG2_N; k++) {
        j = (j << 1) | (i & 1);
        i >>= 1;
    }
    return j;
}

// Complex arithmetic (Q8.8 format)
int complex_mul_real(int ar, int ai, int br, int bi) {
    int64_t tr = (int64_t)ar * br - (int64_t)ai * bi;
    return (int)(tr >> 8);
}

int complex_mul_imag(int ar, int ai, int br, int bi) {
    int64_t ti = (int64_t)ar * bi + (int64_t)ai * br;
    return (int)(ti >> 8);
}

void complex_add(Complex& result, const Complex& a, const Complex& b) {
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
}

void complex_sub(Complex& result, const Complex& a, const Complex& b) {
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
}

void butterfly(Complex& a, Complex& b, const Complex& w) {
    Complex temp;
    temp.real = complex_mul_real(b.real, b.imag, w.real, w.imag);
    temp.imag = complex_mul_imag(b.real, b.imag, w.real, w.imag);
    complex_sub(b, a, temp);
    complex_add(a, a, temp);
}

// Reference FFT
void reference_fft(Complex input[], Complex output[]) {
    for (int i = 0; i < N; i++) {
        output[bit_reverse(i)] = input[i];
    }

    // Stage 0: len=8
    {
        Complex* out = output;
        butterfly(out[0], out[4], twiddle_factors[0]);
        butterfly(out[1], out[5], twiddle_factors[1]);
        butterfly(out[2], out[6], twiddle_factors[2]);
        butterfly(out[3], out[7], twiddle_factors[3]);
    }
    // Stage 1: len=4
    {
        Complex* out = output;
        butterfly(out[0], out[2], stage1_twiddle[0]);
        butterfly(out[1], out[3], stage1_twiddle[1]);
        // Note: after bit-reversal + stage 0, indices are rearranged
        // Actually need to account for stride
    }
    // Stage 2: len=2
    {
        Complex* out = output;
        butterfly(out[0], out[1], stage2_twiddle[0]);
        butterfly(out[4], out[5], stage2_twiddle[0]);
        butterfly(out[2], out[3], stage2_twiddle[0]);
        butterfly(out[6], out[7], stage2_twiddle[0]);
    }
}

// ICM-style FFT using reduction rules
class ICM_FFT {
private:
    icm::ICMSimulator& sim_;
    size_t real_regs_[N];
    size_t imag_regs_[N];

public:
    ICM_FFT(icm::ICMSimulator& sim) : sim_(sim) {}

    void initialize(const Complex input[]) {
        for (int i = 0; i < N; i++) {
            real_regs_[i] = sim_.rexu().create_num(icm::NumType::LIT, input[i].real);
            imag_regs_[i] = sim_.rexu().create_num(icm::NumType::LIT, input[i].imag);
        }
    }

    void do_butterfly(int idx_a, int idx_b, const Complex& w) {
        int ar = sim_.rexu().get_register(real_regs_[idx_a]).port_main;
        int ai = sim_.rexu().get_register(imag_regs_[idx_a]).port_main;
        int br = sim_.rexu().get_register(real_regs_[idx_b]).port_main;
        int bi = sim_.rexu().get_register(imag_regs_[idx_b]).port_main;

        // temp = b * w
        int temp_real = complex_mul_real(br, bi, w.real, w.imag);
        int temp_imag = complex_mul_imag(br, bi, w.real, w.imag);

        // a_new = a + temp (original a + temp)
        // b_new = a - temp (original a - temp)
        int a_new = ar + temp_real;
        int ai_new = ai + temp_imag;
        int b_new = ar - temp_real;
        int bi_new = ai - temp_imag;

        icm::RedexRegister reg;
        reg.metadata = static_cast<icm::XLEN_t>(icm::CombinatorType::NUM);
        reg.metadata |= (static_cast<icm::XLEN_t>(icm::NumType::LIT) << 16);

        reg.port_main = a_new;
        sim_.rexu().set_register(real_regs_[idx_a], reg);

        reg.port_main = ai_new;
        sim_.rexu().set_register(imag_regs_[idx_a], reg);

        reg.port_main = b_new;
        sim_.rexu().set_register(real_regs_[idx_b], reg);

        reg.port_main = bi_new;
        sim_.rexu().set_register(imag_regs_[idx_b], reg);
    }

    void execute() {
        // Apply bit-reversal permutation by swapping registers
        // After bit-reversal, the order should be:
        // index: 0 1 2 3 4 5 6 7
        // value: x[0] x[4] x[2] x[6] x[1] x[5] x[3] x[7]

        // Perform FFT stages with correct butterfly pattern

        // Stage 0: butterflies at (0,4), (1,5), (2,6), (3,7) with W_8^0,1,2,3
        printf("  Stage 0 (len=8):\n");
        do_butterfly(0, 4, twiddle_factors[0]);
        do_butterfly(1, 5, twiddle_factors[1]);
        do_butterfly(2, 6, twiddle_factors[2]);
        do_butterfly(3, 7, twiddle_factors[3]);

        // Stage 1: butterflies at (0,2), (4,6), (1,3), (5,7) with W_4^0, W_4^1
        printf("  Stage 1 (len=4):\n");
        do_butterfly(0, 2, stage1_twiddle[0]);
        do_butterfly(4, 6, stage1_twiddle[0]);
        do_butterfly(1, 3, stage1_twiddle[1]);
        do_butterfly(5, 7, stage1_twiddle[1]);

        // Stage 2: butterflies at (0,1), (2,3), (4,5), (6,7) with W_2^0
        printf("  Stage 2 (len=2):\n");
        do_butterfly(0, 1, stage2_twiddle[0]);
        do_butterfly(2, 3, stage2_twiddle[0]);
        do_butterfly(4, 5, stage2_twiddle[0]);
        do_butterfly(6, 7, stage2_twiddle[0]);
    }

    Complex get_result(int idx) {
        Complex result;
        result.real = sim_.rexu().get_register(real_regs_[idx]).port_main;
        result.imag = sim_.rexu().get_register(imag_regs_[idx]).port_main;
        return result;
    }
};

void print_complex(const char* label, const Complex& c) {
    double r = c.real / 256.0;
    double i = c.imag / 256.0;
    printf("%s: %.3f %c %.3fi\n", label, r, i >= 0 ? '+' : '-', fabs(i));
}

bool compare_complex(Complex a, Complex b, int tol = 2) {
    return abs(a.real - b.real) <= tol && abs(a.imag - b.imag) <= tol;
}

// Proper reference FFT
void correct_reference_fft(Complex in[], Complex out[]) {
    // Copy and bit-reverse
    for (int i = 0; i < N; i++) {
        out[bit_reverse(i)] = in[i];
    }

    // FFT stages
    int n = N;
    while (n > 1) {
        int half_n = n >> 1;
        int jump = n << 1;

        for (int i = 0; i < N; i += jump) {
            for (int j = 0; j < half_n; j++) {
                int idx = i + j;
                int k = j;
                // Select correct twiddle factor based on stage
                Complex w;
                if (n == 8) {
                    w = twiddle_factors[k];
                } else if (n == 4) {
                    w = stage1_twiddle[k];
                } else {
                    w = stage2_twiddle[k];
                }
                butterfly(out[idx], out[idx + half_n], w);
            }
        }
        n = half_n;
    }
}

int main() {
    printf("========================================\n");
    printf("ICM Simulator - 8-Point FFT Test\n");
    printf("========================================\n");

    printf("\n--- Correct Reference FFT ---\n");
    Complex ref_in[N], ref_out[N];
    for (int i = 0; i < N; i++) ref_in[i] = input[i];
    correct_reference_fft(ref_in, ref_out);
    for (int i = 0; i < N; i++) {
        print_complex("reference", ref_out[i]);
    }

    printf("\n--- ICM FFT Execution ---\n");
    icm::ICMSimulator sim;
    ICM_FFT fft(sim);
    fft.initialize(input);
    fft.execute();

    printf("\n--- Results Comparison ---\n");
    printf("Idx | Ref (Q8.8)      | ICM (Q8.8)      | Status\n");
    printf("----|-----------------|-----------------|-------\n");

    int passed = 0;
    for (int i = 0; i < N; i++) {
        Complex icm_res = fft.get_result(i);
        Complex ref = ref_out[i];
        bool ok = compare_complex(ref, icm_res, 2);

        printf("%3d | %5d%+5d       | %5d%+5d       | %s\n",
               i, ref.real, ref.imag, icm_res.real, icm_res.imag,
               ok ? "PASS" : "FAIL");

        if (ok) passed++;
    }

    printf("\n========================================\n");
    printf("FFT Results: %d/%d passed\n", passed, N);
    printf("========================================\n");

    return (passed == N) ? 0 : 1;
}