#ifndef ICM_CORE_CPP
#define ICM_CORE_CPP

#include "icm_core.hpp"
#include <cstdio>

namespace icm {

void RedexExecutionUnit::set_register(size_t addr, const RedexRegister& reg) {
    if (addr < MAX_REGISTERS) {
        registers_[addr] = reg;
    }
}

std::pair<Port, Port> ICMSimulator::find_redex() {
    for (size_t i = active_wires_.size(); i > 0; i--) {
        const auto& wire = active_wires_[i - 1];
        const RedexRegister& reg1 = rexu_.get_register(wire.end1.register_addr);
        const RedexRegister& reg2 = rexu_.get_register(wire.end2.register_addr);

        // Skip if either register is empty (erased)
        if (reg1.metadata == 0 && reg2.metadata == 0) {
            active_wires_.erase(active_wires_.begin() + (i - 1));
            continue;
        }
        if (reg1.metadata == 0 || reg2.metadata == 0) {
            continue;
        }
        // Skip self-connections
        if (wire.end1.register_addr == wire.end2.register_addr) {
            continue;
        }

        CombinatorType type1 = get_type(reg1);
        CombinatorType type2 = get_type(reg2);

        // ERA ~ ERA => VOID (skip, no reduction)
        if (type1 == CombinatorType::ERA && type2 == CombinatorType::ERA) {
            continue;
        }

        // NUM ~ OPR => OPER
        if (type1 == CombinatorType::NUM && type2 == CombinatorType::OPR) {
            return {wire.end1, wire.end2};
        }
        if (type1 == CombinatorType::OPR && type2 == CombinatorType::NUM) {
            return {wire.end1, wire.end2};
        }

        // ERA ~ X => ERAS (except ERA ~ ERA handled above)
        if (type1 == CombinatorType::ERA && type2 != CombinatorType::ERA) {
            return {wire.end1, wire.end2};
        }
        if (type2 == CombinatorType::ERA && type1 != CombinatorType::ERA) {
            return {wire.end1, wire.end2};
        }

        // ANNI: CON~CON, DUP~DUP, OPR~OPR, SWI~SWI, FAN~FAN, BRA~BRA
        if ((type1 == CombinatorType::CON && type2 == CombinatorType::CON) ||
            (type1 == CombinatorType::DUP && type2 == CombinatorType::DUP) ||
            (type1 == CombinatorType::OPR && type2 == CombinatorType::OPR) ||
            (type1 == CombinatorType::SWI && type2 == CombinatorType::SWI) ||
            (type1 == CombinatorType::FAN && type2 == CombinatorType::FAN) ||
            (type1 == CombinatorType::BRA && type2 == CombinatorType::BRA)) {
            return {wire.end1, wire.end2};
        }

        // NUM ~ NUM => VOID (skip, no reduction)
        if (type1 == CombinatorType::NUM && type2 == CombinatorType::NUM) {
            continue;
        }

        // VCON ~ VCON => VOID (skip, no reduction)
        if (type1 == CombinatorType::VCON && type2 == CombinatorType::VCON) {
            continue;
        }

        // CON ~ ERA => ERAS
        if ((type1 == CombinatorType::CON && type2 == CombinatorType::ERA) ||
            (type1 == CombinatorType::ERA && type2 == CombinatorType::CON)) {
            return {wire.end1, wire.end2};
        }
        // DUP ~ ERA => ERAS
        if ((type1 == CombinatorType::DUP && type2 == CombinatorType::ERA) ||
            (type1 == CombinatorType::ERA && type2 == CombinatorType::DUP)) {
            return {wire.end1, wire.end2};
        }
        // CON ~ DUP => COMM
        if ((type1 == CombinatorType::CON && type2 == CombinatorType::DUP) ||
            (type1 == CombinatorType::DUP && type2 == CombinatorType::CON)) {
            return {wire.end1, wire.end2};
        }
        // SWI ~ OPR => COMM
        if ((type1 == CombinatorType::SWI && type2 == CombinatorType::OPR) ||
            (type1 == CombinatorType::OPR && type2 == CombinatorType::SWI)) {
            return {wire.end1, wire.end2};
        }
        // SWI ~ NUM => SWIT
        if ((type1 == CombinatorType::SWI && type2 == CombinatorType::NUM) ||
            (type1 == CombinatorType::NUM && type2 == CombinatorType::SWI)) {
            return {wire.end1, wire.end2};
        }
        // VCON ~ ERA => ERAS
        if ((type1 == CombinatorType::VCON && type2 == CombinatorType::ERA) ||
            (type1 == CombinatorType::ERA && type2 == CombinatorType::VCON)) {
            return {wire.end1, wire.end2};
        }
        // IO ~ ERA => ERAS
        if ((type1 == CombinatorType::IO && type2 == CombinatorType::ERA) ||
            (type1 == CombinatorType::ERA && type2 == CombinatorType::IO)) {
            return {wire.end1, wire.end2};
        }
    }
    return {{0, 0}, {0, 0}};
}

bool ICMSimulator::step() {
    if (reduction_count_ >= max_reductions_) {
        return false;
    }

    auto [port_a, port_b] = find_redex();
    if (port_a.register_addr == 0 && port_b.register_addr == 0) {
        return false;
    }

    if (port_a.register_addr == port_b.register_addr) {
        return false;
    }

    const RedexRegister& reg_a = rexu_.get_register(port_a.register_addr);
    const RedexRegister& reg_b = rexu_.get_register(port_b.register_addr);
    CombinatorType type_a = get_type(reg_a);
    CombinatorType type_b = get_type(reg_b);

    // ANNI: CON~CON, DUP~DUP, OPR~OPR, SWI~SWI, FAN~FAN, BRA~BRA
    if ((type_a == CombinatorType::CON && type_b == CombinatorType::CON) ||
        (type_a == CombinatorType::DUP && type_b == CombinatorType::DUP) ||
        (type_a == CombinatorType::OPR && type_b == CombinatorType::OPR) ||
        (type_a == CombinatorType::SWI && type_b == CombinatorType::SWI) ||
        (type_a == CombinatorType::FAN && type_b == CombinatorType::FAN) ||
        (type_a == CombinatorType::BRA && type_b == CombinatorType::BRA)) {
        RedexRegister era;
        era.metadata = static_cast<XLEN_t>(CombinatorType::ERA);
        rexu_.set_register(port_a.register_addr, era);
        rexu_.set_register(port_b.register_addr, era);
        reduction_count_++;
        return true;
    }

    // ERAS: ERA~X (X != ERA), CON~ERA, DUP~ERA, VCON~ERA, IO~ERA
    if (type_a == CombinatorType::ERA && type_b != CombinatorType::ERA) {
        RedexRegister era;
        era.metadata = static_cast<XLEN_t>(CombinatorType::ERA);
        rexu_.set_register(port_b.register_addr, era);
        reduction_count_++;
        return true;
    }
    if (type_b == CombinatorType::ERA && type_a != CombinatorType::ERA) {
        RedexRegister era;
        era.metadata = static_cast<XLEN_t>(CombinatorType::ERA);
        rexu_.set_register(port_a.register_addr, era);
        reduction_count_++;
        return true;
    }

    // OPER: OPR ~ NUM => compute addition
    // OPR has: port_main = left operand addr, port_aux1 = right operand addr
    if (type_a == CombinatorType::OPR && type_b == CombinatorType::NUM) {
        XLEN_t left_val = get_num_value(rexu_.get_register(reg_a.port_main));
        XLEN_t right_val = get_num_value(rexu_.get_register(reg_a.port_aux1));
        XLEN_t result = left_val + right_val;

        RedexRegister result_reg;
        result_reg.metadata = static_cast<XLEN_t>(CombinatorType::NUM);
        result_reg.metadata |= (static_cast<XLEN_t>(NumType::LIT) << 16);
        result_reg.port_main = result;

        rexu_.set_register(port_a.register_addr, result_reg);
        reduction_count_++;
        return true;
    }
    if (type_a == CombinatorType::NUM && type_b == CombinatorType::OPR) {
        XLEN_t left_val = get_num_value(rexu_.get_register(reg_b.port_main));
        XLEN_t right_val = get_num_value(rexu_.get_register(reg_b.port_aux1));
        XLEN_t result = left_val + right_val;

        RedexRegister result_reg;
        result_reg.metadata = static_cast<XLEN_t>(CombinatorType::NUM);
        result_reg.metadata |= (static_cast<XLEN_t>(NumType::LIT) << 16);
        result_reg.port_main = result;

        rexu_.set_register(port_b.register_addr, result_reg);
        reduction_count_++;
        return true;
    }

    // COMM: CON~DUP - swap and mark as reduced (set to ERA to stop re-finding)
    if ((type_a == CombinatorType::CON && type_b == CombinatorType::DUP) ||
        (type_a == CombinatorType::DUP && type_b == CombinatorType::CON)) {
        RedexRegister era;
        era.metadata = static_cast<XLEN_t>(CombinatorType::ERA);
        rexu_.set_register(port_a.register_addr, era);
        rexu_.set_register(port_b.register_addr, era);
        reduction_count_++;
        return true;
    }

    // SWIT: SWI ~ NUM - switch on predicate
    if ((type_a == CombinatorType::SWI && type_b == CombinatorType::NUM) ||
        (type_a == CombinatorType::NUM && type_b == CombinatorType::SWI)) {
        RedexRegister era;
        era.metadata = static_cast<XLEN_t>(CombinatorType::ERA);
        rexu_.set_register(port_a.register_addr, era);
        rexu_.set_register(port_b.register_addr, era);
        reduction_count_++;
        return true;
    }

    // COMM: SWI ~ OPR - mark as reduced
    if ((type_a == CombinatorType::SWI && type_b == CombinatorType::OPR) ||
        (type_a == CombinatorType::OPR && type_b == CombinatorType::SWI)) {
        RedexRegister era;
        era.metadata = static_cast<XLEN_t>(CombinatorType::ERA);
        rexu_.set_register(port_a.register_addr, era);
        rexu_.set_register(port_b.register_addr, era);
        reduction_count_++;
        return true;
    }

    return false;
}

void ICMSimulator::run() {
    while (step()) {
    }
}

void ICMSimulator::print_state() const {
    std::printf("ICM Simulator State (Reduction count: %d)\n", reduction_count_);
    std::printf("Total registers: %zu\n", rexu_.get_register_count());
    std::printf("----------------------------------------\n");
    for (size_t i = 0; i < rexu_.get_register_count(); ++i) {
        const RedexRegister& reg = rexu_.get_register(i);
        CombinatorType type = get_type(reg);
        std::printf("[%4zu] ", i);
        switch (type) {
            case CombinatorType::CON:
                std::printf("CON  (main=%u, aux1=%u, aux2=%u)", reg.port_main, reg.port_aux1, reg.port_aux2);
                break;
            case CombinatorType::DUP:
                std::printf("DUP  (main=%u, aux1=%u, aux2=%u)", reg.port_main, reg.port_aux1, reg.port_aux2);
                break;
            case CombinatorType::ERA:
                std::printf("ERA");
                break;
            case CombinatorType::VAR:
                std::printf("VAR  (main=%u)", reg.port_main);
                break;
            case CombinatorType::NUM:
                std::printf("NUM  (val=%u, type=%d)", reg.port_main, static_cast<int>(get_num_type(reg)));
                break;
            case CombinatorType::OPR:
                std::printf("OPR  (left=%u, right=%u)", reg.port_main, reg.port_aux1);
                break;
            case CombinatorType::SWI:
                std::printf("SWI  (main=%u, aux1=%u)", reg.port_main, reg.port_aux1);
                break;
            case CombinatorType::FAN:
                std::printf("FAN  (main=%u, aux1=%u, aux2=%u)", reg.port_main, reg.port_aux1, reg.port_aux2);
                break;
            case CombinatorType::BRA:
                std::printf("BRA  (main=%u, aux1=%u, aux2=%u)", reg.port_main, reg.port_aux1, reg.port_aux2);
                break;
            case CombinatorType::VCON:
                std::printf("VCON (main=%u, aux1=%u, aux2=%u)", reg.port_main, reg.port_aux1, reg.port_aux2);
                break;
            case CombinatorType::IO:
                std::printf("IO   (main=%u, aux1=%u, aux2=%u)", reg.port_main, reg.port_aux1, reg.port_aux2);
                break;
            default:
                if (reg.metadata == 0) {
                    std::printf("EMPTY");
                } else {
                    std::printf("UNKNOWN (meta=0x%08X)", reg.metadata);
                }
                break;
        }
        std::printf("\n");
    }
}

void ICMSimulator::print_wires() const {
    std::printf("Active wires: %zu\n", active_wires_.size());
    for (size_t i = 0; i < active_wires_.size(); ++i) {
        const auto& wire = active_wires_[i];
        std::printf("[%zu] (%zu.%zu) <-> (%zu.%zu)\n",
                    i, wire.end1.register_addr, wire.end1.port_index,
                    wire.end2.register_addr, wire.end2.port_index);
    }
}

} // namespace icm

#endif // ICM_CORE_CPP