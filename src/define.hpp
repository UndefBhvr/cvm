#ifndef DEFINE_HPP
#define DEFINE_HPP

#include <utility>
#include <map>

enum class Type {
    Nullary,
    Unary,
    Binary
};

class Combinator {
public:
    Type type() const { 
        return _type; 
    }
    static Combinator CON, OPX, DUP, ERA, NUM, VCON, VAR;

private:
    Combinator(Type t) : _type(t) {}
    Type _type;
};

// Static member definitions
Combinator Combinator::CON = Combinator(Type::Binary);
Combinator Combinator::OPX = Combinator(Type::Binary);
Combinator Combinator::DUP = Combinator(Type::Binary);
Combinator Combinator::ERA = Combinator(Type::Nullary);
Combinator Combinator::NUM = Combinator(Type::Nullary);
Combinator Combinator::VCON = Combinator(Type::Binary);
Combinator Combinator::VAR = Combinator(Type::Unary);

enum class Operation {
    ANNI,
    COMM,
    ERAS,
    SWI,
    OPR,
    VOID,
    LINK,
    REPC
};

// Comparator class for std::pair<Combinator, Combinator>
struct CombinatorPairComparator {
    bool operator()(const std::pair<Combinator, Combinator>& lhs, const std::pair<Combinator, Combinator>& rhs) const {
        // Compare the first Combinator
        if (lhs.first.type() != rhs.first.type()) {
            return lhs.first.type() < rhs.first.type();
        }
        // If the first Combinators are the same, compare the second Combinator
        return lhs.second.type() < rhs.second.type();
    }
};

// Mapping from pairs of combinators to operations
std::map<std::pair<Combinator, Combinator>, Operation, CombinatorPairComparator> operation = {
    {{Combinator::CON, Combinator::CON}, Operation::ANNI},
    {{Combinator::CON, Combinator::OPX}, Operation::ANNI},
    {{Combinator::CON, Combinator::DUP}, Operation::COMM},
    {{Combinator::CON, Combinator::ERA}, Operation::ERAS},
    {{Combinator::CON, Combinator::NUM}, Operation::SWI},
    {{Combinator::CON, Combinator::VCON}, Operation::ANNI},
    {{Combinator::CON, Combinator::VAR}, Operation::LINK},
    {{Combinator::OPX, Combinator::CON}, Operation::ANNI},
    {{Combinator::OPX, Combinator::OPX}, Operation::ANNI},
    {{Combinator::OPX, Combinator::DUP}, Operation::COMM},
    {{Combinator::OPX, Combinator::ERA}, Operation::ERAS},
    {{Combinator::OPX, Combinator::NUM}, Operation::OPR},
    {{Combinator::OPX, Combinator::VCON}, Operation::OPR},
    {{Combinator::OPX, Combinator::VAR}, Operation::LINK},
    {{Combinator::DUP, Combinator::CON}, Operation::COMM},
    {{Combinator::DUP, Combinator::OPX}, Operation::COMM},
    {{Combinator::DUP, Combinator::DUP}, Operation::REPC},
    {{Combinator::DUP, Combinator::ERA}, Operation::ERAS},
    {{Combinator::DUP, Combinator::NUM}, Operation::ERAS},
    {{Combinator::DUP, Combinator::VCON}, Operation::ERAS},
    {{Combinator::DUP, Combinator::VAR}, Operation::LINK},
    {{Combinator::ERA, Combinator::CON}, Operation::ERAS},
    {{Combinator::ERA, Combinator::OPX}, Operation::ERAS},
    {{Combinator::ERA, Combinator::DUP}, Operation::ERAS},
    {{Combinator::ERA, Combinator::ERA}, Operation::VOID},
    {{Combinator::ERA, Combinator::NUM}, Operation::VOID},
    {{Combinator::ERA, Combinator::VCON}, Operation::VOID},
    {{Combinator::ERA, Combinator::VAR}, Operation::LINK},
    {{Combinator::NUM, Combinator::CON}, Operation::SWI},
    {{Combinator::NUM, Combinator::OPX}, Operation::OPR},
    {{Combinator::NUM, Combinator::DUP}, Operation::ERAS},
    {{Combinator::NUM, Combinator::ERA}, Operation::VOID},
    {{Combinator::NUM, Combinator::NUM}, Operation::VOID},
    {{Combinator::NUM, Combinator::VCON}, Operation::VOID},
    {{Combinator::NUM, Combinator::VAR}, Operation::LINK},
    {{Combinator::VCON, Combinator::CON}, Operation::ANNI},
    {{Combinator::VCON, Combinator::OPX}, Operation::OPR},
    {{Combinator::VCON, Combinator::DUP}, Operation::ERAS},
    {{Combinator::VCON, Combinator::ERA}, Operation::ERAS},
    {{Combinator::VCON, Combinator::NUM}, Operation::ERAS},
    {{Combinator::VCON, Combinator::VCON}, Operation::ANNI},
    {{Combinator::VCON, Combinator::VAR}, Operation::LINK},
    {{Combinator::VAR, Combinator::CON}, Operation::LINK},
    {{Combinator::VAR, Combinator::OPX}, Operation::LINK},
    {{Combinator::VAR, Combinator::DUP}, Operation::LINK},
    {{Combinator::VAR, Combinator::ERA}, Operation::LINK},
    {{Combinator::VAR, Combinator::NUM}, Operation::LINK},
    {{Combinator::VAR, Combinator::VCON}, Operation::LINK},
    {{Combinator::VAR, Combinator::VAR}, Operation::LINK}
};

#endif // DEFINE_HPP