struct Log
{
	time_t timestamp;
	int instructionLine;
	string display;
	// formulate this shit
};

struct Variable
{
	uint16_t value;
};

enum InstructionType
{
	PRINT,
	DECLARE,
	ADD,
	SUBTRACT,
	SLEEP,
	FOR
};

struct Instruction
{
	InstructionType type;
	std::vector<std::string> args;
	std::vector<Instruction> subinstructions;
};

class Process
{
	string processName;
	int id;
	static int nextId;
	int core;
	bool finished;
	vector<Instruction> instructions;
	unordered_map<string, Variable> vars;
	int instructionPointer;
	vector<Log> logs;

public:
	Process(string name, int assignedCore) : processName(name),
		core(assignedCore),
		id(nextId++),
		finished(false),
		instructionPointer(0)
	{}
	Process(string name, int assignedCore, int minIns, int maxIns) : processName(name),
		core(assignedCore),
		id(nextId++),
		finished(false),
		instructionPointer(0)
	{
		generateRandomInstructions(minIns, maxIns);
	}

	void setFinished(bool val) { finished = val; }

	void executeInstruction(Instruction &instr)
	{
		switch (instr.type)
		{
		case DECLARE:
			vars[instr.args[0]].value = getValue(instr.args[1]);
			break;
		case ADD:
			vars[instr.args[0]].value = getValue(instr.args[1]) + getValue(instr.args[2]);
			break;
		case SUBTRACT:
			vars[instr.args[0]].value = getValue(instr.args[1]) - getValue(instr.args[2]);
			break;
		}
	}

	bool isFinished() const { return finished; }
	string getName() const { return processName; }
	int getId() const { return id; }
	int getCurrentInstructionLine() const { return instructionPointer - instructions.size(); }
	int getInstructionSize() const { return instructions.size(); }

private:
	void generateRandomInstructions(int minIns, int maxIns)
	{
		srand(0);

		long long int numInstr = rand() % (maxIns - (minIns + 1)) + minIns;

		vector<std::string> varNames = {"var1", "var2", "var3", "var4"};

		for (const auto &varName : varNames)
		{
			int randomValue = rand() % 101;
			instructions.push_back({DECLARE, {varName, std::to_string(randomValue)}});
		}

		int remainingInstructions = numInstr - varNames.size();
		if (remainingInstructions <= 0)
			return;

		for (int i = 0; i < remainingInstructions; ++i)
		{
			Instruction instr;
			int type = rand() % 6;

			string v1 = varNames[rand() % varNames.size()];
			string v2 = varNames[rand() % varNames.size()];
			string v3_val = std::to_string(rand() % 101); // 0-100

			switch (type)
			{
			case 0: // PRINT
				instr = {PRINT, {"Hello world from "}};
				break;
			case 1: // DECLARE
				instr = {DECLARE, {v1, v3_val}};
				break;
			case 2: // ADD
				instr = {ADD, {v1, v2, v3_val}};
				break;
			case 3: // SUBTRACT
				instr = {SUBTRACT, {v1, v2, std::to_string(rand() % 11)}};
				break;
			case 4: // SLEEP
				instr = {SLEEP, {std::to_string((rand() % 5) + 1)}};
				break;
			case 5: // FOR
				instr = {FOR, {std::to_string((rand() % 3) + 2)}};
				instr.subinstructions.push_back({ADD, {v1, v1, "1"}});
				break;
			}
			instructions.push_back(instr);
		}
	}
	uint16_t getValue(std::string &token)
	{
		if (vars.count(token))
			return vars[token].value;
		return static_cast<uint16_t>(std::stoi(token));
	}
};
int Process::nextId = 0;


