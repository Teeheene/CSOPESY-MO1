#include <string>
#include <vector>
#include <sstream>

struct Instruction {
    std::string operation;
    std::vector<std::string> arguments;

    Instruction() = default;

    Instruction(const std::string& op, const std::vector<std::string>& args = {})
        : operation(op), arguments(args) {}

    std::string toString() const {
        std::stringstream ss;
        ss << operation;
        for (const auto& arg : arguments) {
            ss << " " << arg;
        }
        return ss.str();
    }
};