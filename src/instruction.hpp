#include <iostream>
#include <string>
#include <vector>
#include <map>        // For variable memory
#include <sstream>    // For string manipulation

struct Instruction {
    string operation;
    vector<string> arguments;
	bool output;

    Instruction() = default;
    Instruction(const string& op, const vector<string>& args = {})
        : operation(op), arguments(args) {}

    string toString() const {
        stringstream ss;
        ss << operation;
        for (const auto& arg : arguments) {
            ss << " " << arg;
        }
        return ss.str();
    }
};
