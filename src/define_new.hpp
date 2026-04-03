#ifndef DEFINE_NEW_HPP
#define DEFINE_NEW_HPP

#include <map>
#include <utility>

// Simplified type enumeration
enum class Type {
    Nullary,
    Unary,
    Binary
};

// Combinator types as simple enum
enum class CombinatorType {
    CON,    // Constructor
    OPX,    // Operation
    DUP,    // Duplicate
    ERA,    // Erase
    NUM,    // Number
    VCON,   // Vector Constructor
    VAR     // Variable
};

// Operation types
enum class Operation {
    ANNI,   // Annihilation
    COMM,   // Commutation
    ERAS,   // Erasure
    SWI,    // Switch
    OPR,    // Operator
    VOID,   // Void
    LINK,   // Link
    REPC    // Replication
};

// Metadata for each combinator: type and operation behavior
struct CombinatorInfo {
    Type type;
};

// Combinator metadata table
constexpr CombinatorInfo combinator_info[] = {
    {Type::Binary},   // CON
    {Type::Binary},   // OPX
    {Type::Binary},   // DUP
    {Type::Nullary},  // ERA
    {Type::Nullary},  // NUM
    {Type::Binary},   // VCON
    {Type::Unary}     // VAR
};

// Simple pair hash and equality for enums
using CombinatorPair = std::pair<CombinatorType, CombinatorType>;

struct CombinatorPairHash {
    size_t operator()(const CombinatorPair& p) const {
        return (static_cast<size_t>(p.first) << 4) | static_cast<size_t>(p.second);
    }
};

// Operation lookup table indexed by combinator pair
// Using unordered_map for O(1) lookup
std::map<CombinatorPair, Operation> interaction_table = {
    // CON interactions
    {{CombinatorType::CON, CombinatorType::CON}, Operation::ANNI},
    {{CombinatorType::CON, CombinatorType::OPX}, Operation::ANNI},
    {{CombinatorType::CON, CombinatorType::DUP}, Operation::COMM},
    {{CombinatorType::CON, CombinatorType::ERA}, Operation::ERAS},
    {{CombinatorType::CON, CombinatorType::NUM}, Operation::SWI},
    {{CombinatorType::CON, CombinatorType::VCON}, Operation::ANNI},
    {{CombinatorType::CON, CombinatorType::VAR}, Operation::LINK},
    
    // OPX interactions
    {{CombinatorType::OPX, CombinatorType::CON}, Operation::ANNI},
    {{CombinatorType::OPX, CombinatorType::OPX}, Operation::ANNI},
    {{CombinatorType::OPX, CombinatorType::DUP}, Operation::COMM},
    {{CombinatorType::OPX, CombinatorType::ERA}, Operation::ERAS},
    {{CombinatorType::OPX, CombinatorType::NUM}, Operation::OPR},
    {{CombinatorType::OPX, CombinatorType::VCON}, Operation::OPR},
    {{CombinatorType::OPX, CombinatorType::VAR}, Operation::LINK},
    
    // DUP interactions
    {{CombinatorType::DUP, CombinatorType::CON}, Operation::COMM},
    {{CombinatorType::DUP, CombinatorType::OPX}, Operation::COMM},
    {{CombinatorType::DUP, CombinatorType::DUP}, Operation::REPC},
    {{CombinatorType::DUP, CombinatorType::ERA}, Operation::ERAS},
    {{CombinatorType::DUP, CombinatorType::NUM}, Operation::ERAS},
    {{CombinatorType::DUP, CombinatorType::VCON}, Operation::ERAS},
    {{CombinatorType::DUP, CombinatorType::VAR}, Operation::LINK},
    
    // ERA interactions
    {{CombinatorType::ERA, CombinatorType::CON}, Operation::ERAS},
    {{CombinatorType::ERA, CombinatorType::OPX}, Operation::ERAS},
    {{CombinatorType::ERA, CombinatorType::DUP}, Operation::ERAS},
    {{CombinatorType::ERA, CombinatorType::ERA}, Operation::VOID},
    {{CombinatorType::ERA, CombinatorType::NUM}, Operation::VOID},
    {{CombinatorType::ERA, CombinatorType::VCON}, Operation::VOID},
    {{CombinatorType::ERA, CombinatorType::VAR}, Operation::LINK},
    
    // NUM interactions
    {{CombinatorType::NUM, CombinatorType::CON}, Operation::SWI},
    {{CombinatorType::NUM, CombinatorType::OPX}, Operation::OPR},
    {{CombinatorType::NUM, CombinatorType::DUP}, Operation::ERAS},
    {{CombinatorType::NUM, CombinatorType::ERA}, Operation::VOID},
    {{CombinatorType::NUM, CombinatorType::NUM}, Operation::VOID},
    {{CombinatorType::NUM, CombinatorType::VCON}, Operation::VOID},
    {{CombinatorType::NUM, CombinatorType::VAR}, Operation::LINK},
    
    // VCON interactions
    {{CombinatorType::VCON, CombinatorType::CON}, Operation::ANNI},
    {{CombinatorType::VCON, CombinatorType::OPX}, Operation::OPR},
    {{CombinatorType::VCON, CombinatorType::DUP}, Operation::ERAS},
    {{CombinatorType::VCON, CombinatorType::ERA}, Operation::ERAS},
    {{CombinatorType::VCON, CombinatorType::NUM}, Operation::ERAS},
    {{CombinatorType::VCON, CombinatorType::VCON}, Operation::ANNI},
    {{CombinatorType::VCON, CombinatorType::VAR}, Operation::LINK},
    
    // VAR interactions (all map to LINK)
    {{CombinatorType::VAR, CombinatorType::CON}, Operation::LINK},
    {{CombinatorType::VAR, CombinatorType::OPX}, Operation::LINK},
    {{CombinatorType::VAR, CombinatorType::DUP}, Operation::LINK},
    {{CombinatorType::VAR, CombinatorType::ERA}, Operation::LINK},
    {{CombinatorType::VAR, CombinatorType::NUM}, Operation::LINK},
    {{CombinatorType::VAR, CombinatorType::VCON}, Operation::LINK},
    {{CombinatorType::VAR, CombinatorType::VAR}, Operation::LINK}
};

// Helper function for safe lookups
inline Operation get_operation(CombinatorType lhs, CombinatorType rhs) {
    auto it = interaction_table.find({lhs, rhs});
    return (it != interaction_table.end()) ? it->second : Operation::VOID;
}

// Helper function to get combinator type info
inline Type get_combinator_type(CombinatorType ct) {
    return combinator_info[static_cast<int>(ct)].type;
}

#endif // DEFINE_NEW_HPP
