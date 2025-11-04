#include <iostream>
#include <string>
#include <vector>
#include <map>        // For variable memory
#include <sstream>    // For string manipulation

struct Instruction {
    std::string operation;
    std::vector<std::string> arguments;
	bool output;

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
    
    // --- FUNCTIONS YOU ASKED FOR ---
    bool isOutput() const {
        return output;
    }

    std::string getOutput() const {
        std::stringstream ss;
        for (const auto& arg : arguments) {
            ss << arg << " ";
        }
        return ss.str();
    }
};


class Process {
	int pid;
	int instructionPointer;             
	vector<Instruction> instructions;
    map<string, int> memory;
    int sleepTimer;

public:
	Process(int id) :
		pid(id),
		instructionPointer(0),
        sleepTimer(0) 
	{}

	Process() :
		pid(-1), 
		instructionPointer(0),
        sleepTimer(0) 
	{}

	void addInstruction(const Instruction &instr) { 
		instructions.push_back(instr);
	}

	bool hasRemainingInstructions() {
		return instructionPointer < instructions.size();
	}

    int getValue(const string& key) {
        if (memory.find(key) != memory.end()) {
            return memory[key];
        }
        try {
            return stoi(key); 
        } catch (...) {
            return 0; // Default to 0
        }
    }

    void setValue(const string& key, int value) {
        memory[key] = value; // <-- SAVES THE VALUE
    }

	void executeNextInstruction(int core) {
		if(!hasRemainingInstructions()) { return; }

        if (sleepTimer > 0) {
            sleepTimer--;
            return;
        }
        Instruction instr = instructions[instructionPointer];
        
        const string& op = instr.operation;
        const vector<string>& args = instr.arguments;
        
        if (op == "DECLARE") {
            if (args.size() >= 1) {
                int initialValue = (args.size() >= 2) ? getValue(args[1]) : 0;
                setValue(args[0], initialValue); 
            }
        } 
        else if (op == "ADD") {
            if (args.size() >= 3) {
                int val2 = getValue(args[1]);
                int val3 = getValue(args[2]);
                int result = val2 + val3;
                setValue(args[0], result);
            }
        }
        else if (op == "SUBTRACT") {
            if (args.size() >= 3) {
                int val2 = getValue(args[1]);
                int val3 = getValue(args[2]);
                int result = val2 - val3;
                setValue(args[0], result);
            }
        }
        else if (op == "PRINT") {
            instr.output = true;;
        }
	
        else if (op == "SLEEP") { 
            if (args.size() >= 1) {
                sleepTimer = getValue(args[0]);
            }
        }
        
		instructionPointer++;
	}
	
	int getPid() { return pid; }
	int getInstructionCount() { return instructions.size(); }
	int getInstructionPointer() { return instructionPointer; }
    bool isAsleep() const { return sleepTimer > 0; } 
	
	string getProgress() { 
		return to_string(instructionPointer) + " / " + to_string(instructions.size()); 
	}

	int getCore() {
		return -1; 
	}
};

vector<Instruction> generateRandomInstruction() {
	static vector<string> ops = {"DECLARE", "ADD", "SUBTRACT", "PRINT", "SLEEP", "FOR"};
    string op = ops[rand() % ops.size()];
    vector<string> args;

    string var1 = "VAR" + to_string(rand() % 3);
    string var2 = "VAR" + to_string(rand() % 3);
    string var3 = "VAR" + to_string(rand() % 3);
    string literal = to_string(rand() % 50 + 1); 

	vector<Instruction> instructions;

    if (op == "DECLARE") {
        args.push_back(var1);
        args.push_back(literal); 
    } else if (op == "ADD" || op == "SUBTRACT") {
        args.push_back(var1); 
        args.push_back(var2); 
        args.push_back((rand() % 2 == 0) ? var3 : literal); 
    } else if (op == "PRINT") {
        args.push_back("Hello World from ");
    } else if (op == "SLEEP") {
        args.push_back(to_string(rand() % 3 + 1)); 
    } else if (op == "FOR") {
		int loopCount = rand() % 3 + 1;

		
		instructions = processForLoop(loopCount);

		return instructions;
	}
    Instruction instr = Instruction(op, args);

	instructions.push_back(instr);

	return instructions;
}

vector<Instruction> processForLoop(int loopCount) {
    vector<Instruction> loopInstructions;

    loopInstructions.push_back(Instruction("FOR", {"ADD, PRINT"}));
	
    vector<Instruction> loopBody;
    loopBody.push_back(Instruction("ADD", {"VAR1", "VAR1", "1"}));
    loopBody.push_back(Instruction("PRINT",{"VAR 1 = "}));

    for (int i = 0; i < loopCount; i++) {
        for (const auto& instr : loopBody) {
            loopInstructions.push_back(instr);
        }
    }

    return loopInstructions;
}
