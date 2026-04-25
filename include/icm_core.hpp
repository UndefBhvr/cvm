#ifndef ICM_CORE_HPP
#define ICM_CORE_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <stdexcept>

namespace icm {

using XLEN_t = uint32_t;
constexpr XLEN_t DEFAULT_XLEN = 32;

enum class CombinatorType : XLEN_t {
    CON  = 0x0001,
    DUP  = 0x0002,
    ERA  = 0x0003,
    VAR  = 0x0004,
    NUM  = 0x0005,
    OPR  = 0x0006,
    SWI  = 0x0007,
    FAN  = 0x0008,
    BRA  = 0x0009,
    VCON = 0x0010,
    IO   = 0x0011,
};

enum class NumType : XLEN_t {
    LIT = 0,
    SYM = 1,
    PAR = 2,
};

struct Port {
    size_t register_addr = 0;
    size_t port_index = 0;
};

struct Wire {
    Port end1;
    Port end2;
};

struct RedexRegister {
    XLEN_t metadata = 0;
    XLEN_t port_main = 0;
    XLEN_t port_aux1 = 0;
    XLEN_t port_aux2 = 0;

    RedexRegister() = default;
    RedexRegister(XLEN_t meta, XLEN_t main = 0, XLEN_t aux1 = 0, XLEN_t aux2 = 0)
        : metadata(meta), port_main(main), port_aux1(aux1), port_aux2(aux2) {}

    bool is_nullary() const {
        CombinatorType type = static_cast<CombinatorType>(metadata & 0xFFFF);
        return type == CombinatorType::NUM || type == CombinatorType::ERA;
    }
};

class RedexExecutionUnit {
public:
    static constexpr size_t MAX_REGISTERS = 1024;

private:
    std::array<RedexRegister, MAX_REGISTERS> registers_;
    size_t register_count_ = 0;
    std::vector<Wire> wires_;

public:
    size_t allocate_register(const RedexRegister& reg) {
        if (register_count_ >= MAX_REGISTERS) {
            throw std::runtime_error("Out of registers");
        }
        size_t addr = register_count_++;
        registers_[addr] = reg;
        return addr;
    }

    size_t create_combinator(CombinatorType type, size_t main = 0, size_t aux1 = 0, size_t aux2 = 0) {
        RedexRegister reg(static_cast<XLEN_t>(type), main, aux1, aux2);
        return allocate_register(reg);
    }

    size_t create_num(NumType num_type, XLEN_t value) {
        RedexRegister reg;
        reg.metadata = static_cast<XLEN_t>(CombinatorType::NUM);
        reg.metadata |= (static_cast<XLEN_t>(num_type) << 16);
        reg.port_main = value;
        return allocate_register(reg);
    }

    size_t create_opr(size_t left, size_t right, const std::string& op = "+") {
        RedexRegister reg;
        reg.metadata = static_cast<XLEN_t>(CombinatorType::OPR);
        reg.port_main = left;
        reg.port_aux1 = right;
        (void)op;
        return allocate_register(reg);
    }

    size_t create_con(size_t main, size_t aux1, size_t aux2) {
        return create_combinator(CombinatorType::CON, main, aux1, aux2);
    }

    size_t create_dup(size_t main, size_t aux1, size_t aux2) {
        return create_combinator(CombinatorType::DUP, main, aux1, aux2);
    }

    size_t create_era() {
        return create_combinator(CombinatorType::ERA);
    }

    void add_wire(size_t reg1, size_t port1, size_t reg2, size_t port2) {
        wires_.push_back({{reg1, port1}, {reg2, port2}});
    }

    const RedexRegister& get_register(size_t addr) const { return registers_[addr]; }
    RedexRegister& get_register(size_t addr) { return registers_[addr]; }
    size_t get_register_count() const { return register_count_; }
    void set_register(size_t addr, const RedexRegister& reg);
    const std::vector<Wire>& get_wires() const { return wires_; }

    void erase_register(size_t addr) {
        if (addr < MAX_REGISTERS) {
            RedexRegister empty;
            registers_[addr] = empty;
        }
    }
};

struct ReductionResult {
    bool did_reduce = false;
    std::string rule_used;
    size_t new_addr = 0;
};

class ICMSimulator {
private:
    RedexExecutionUnit rexu_;
    std::vector<Wire> active_wires_;
    int max_reductions_ = 100000;
    int reduction_count_ = 0;

    CombinatorType get_type(const RedexRegister& reg) const {
        return static_cast<CombinatorType>(reg.metadata & 0xFFFF);
    }

    NumType get_num_type(const RedexRegister& reg) const {
        return static_cast<NumType>((reg.metadata >> 16) & 0xFFFF);
    }

    XLEN_t get_num_value(const RedexRegister& reg) const {
        return reg.port_main;
    }

    std::string get_opr_op(const RedexRegister& reg) const {
        (void)reg;
        return "+";
    }

    std::pair<Port, Port> find_redex();

public:
    void set_max_reductions(int max) { max_reductions_ = max; }

    void add_wire(size_t reg1, size_t port1, size_t reg2, size_t port2) {
        active_wires_.push_back({{reg1, port1}, {reg2, port2}});
    }

    void connect_wires() {
        for (size_t i = 0; i < rexu_.get_register_count(); ++i) {
            const RedexRegister& reg = rexu_.get_register(i);
            CombinatorType type = get_type(reg);
            if (type == CombinatorType::NUM || type == CombinatorType::ERA) continue;
            if (reg.port_main != i) {
                add_wire(i, 0, reg.port_main, 0);
            }
            if (type != CombinatorType::OPR) {
                if (reg.port_aux1 != 0) add_wire(i, 1, reg.port_aux1, 0);
                if (reg.port_aux2 != 0) add_wire(i, 2, reg.port_aux2, 0);
            }
        }
    }

    size_t load_program(const std::vector<RedexRegister>& program) {
        size_t entry = 0;
        for (const auto& reg : program) {
            if (entry == 0) {
                entry = rexu_.allocate_register(reg);
            } else {
                rexu_.allocate_register(reg);
            }
        }
        return entry;
    }

    bool step();
    void run();
    int get_reduction_count() const { return reduction_count_; }

    RedexExecutionUnit& rexu() { return rexu_; }
    const RedexExecutionUnit& rexu() const { return rexu_; }

    void print_state() const;
    void print_wires() const;
};

} // namespace icm

#endif // ICM_CORE_HPP