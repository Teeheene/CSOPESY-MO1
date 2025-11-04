using namespace std;

struct Log {
	time_t timestamp;
	int core;
	string instr;

	Log(int core_, string instr_) {
		timestamp = time(nullptr);
		core = core_;
		instr = instr_;
	}

	void print() {
		//get timestamp adjusted to local time
		char strTime[100];
		tm* translTimestamp = localtime(&timestamp);
		strftime(strTime, sizeof(strTime), "%m/%d/%Y %I:%M:%S%p", translTimestamp);

		//print the log details
		cout << "(" << strTime << ")" << " Core:" << core 
			<< " \"" << instr << "\"" << endl;
	}
};

class Process {
	string name;
	int pid;
	int instructionPointer;
	//vector<Instruction> instructions;
	vector<string> instructions //temporary for testing
	vector<unique_ptr<Log>> logs;
	//for shared processor resources 
	mutex procMtx;

public:
	Process(string name_, int pid_) :
		name(name_),
		pid(pid_),
		instructionPointer(0)
	{}

	Process() :
		instructionPointer(0)
	{}

	void addInstruction(Instruction &instr) {
		instructions.push_back(instr);
	}

	bool hasRemainingInstructions() {
		lock_guard<mutex> lock(procMtx);
		return instructionPointer < instructions.size();
	}

	void executeNextInstruction(int core) {
		lock_guard<mutex> lock(procMtx);
		if(!hasRemainingInstructions()) { return; }
		//executeInstruction(instructions[instructionPointer]);

		//logs ONLY output instructions (e.g. PRINT("meow"))
		if(Instruction.isOutput())
			logs.push_back(make_unique<Log>(core, instructions[instructionPointer].getOutput()));

		//move the pointer forward
		instructionPointer++;
	}

	void printLogs() {
		lock_guard<mutex> lock(procMtx);
		cout << "ID: " << pid << endl;
		cout << "Logs:" << endl;
		for(const auto& log : logs) {
			log->print();
		}
	}
	
	//getters
	int getPid() { return pid; }
	int getInstructionCount() { return instructions.size(); }
	int getInstructionPointer() { return instructionPointer; }
};

