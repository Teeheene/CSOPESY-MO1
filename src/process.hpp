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
	int pid;
	int instructionPointer;
	vector<string> instructions;
	vector<unique_ptr<Log>> logs;
	mutex logMtx;

public:
	Process(int id) :
		pid(id),
		instructionPointer(0)
	{}

	Process() :
		instructionPointer(0)
	{}

	void addInstruction(const string &instr) {
		instructions.push_back(instr);
	}

	bool hasRemainingInstructions() {
		return instructionPointer < instructions.size();
	}

	void executeNextInstruction(int core) {
		lock_guard<mutex> lock(logMtx);
		if(!hasRemainingInstructions()) { return; }

		//log the current instruction
		logs.push_back(make_unique<Log>(core, instructions[instructionPointer]));

		//move the pointer forward
		instructionPointer++;
	}

	void printLogs() {
		lock_guard<mutex> lock(logMtx);
		cout << "ID: " << pid << endl;
		cout << "Logs:" << endl;
		for(const auto& log : logs) {
			log->print();
		}
	}
	
	int getPid() { return pid; }
	int getInstructionCount() { return instructions.size(); }
	int getInstructionPointer() { return instructionPointer; }
};



/*
class Process {
	int pid;
	int instructionPointer;
	vector<string> instructions;
	vector<unique_ptr<Log>> logs;
	mutex logMtx;

public:
	Process(int id) :
		pid(id),
		instructionPointer(0)
	{}

	Process() :
		instructionPointer(0)
	{}

	void addInstruction(const string &instr) {
		instructions.push_back(instr);
	}

	bool hasRemainingInstructions() {
		return instructionPointer < instructions.size();
	}

	void executeNextInstruction(int core) {
		lock_guard<mutex> lock(logMtx);
		if(!hasRemainingInstructions()) { return; }

		//log the current instruction
		logs.push_back(make_unique<Log>(core, instructions[instructionPointer]));

		//move the pointer forward
		instructionPointer++;
	}

	void printLogs() {
		lock_guard<mutex> lock(logMtx);
		cout << "ID: " << pid << endl;
		cout << "Logs:" << endl;
		for(const auto& log : logs) {
			log->print();
		}
	}
	
	int getPid() { return pid; }
	int getInstructionCount() { return instructions.size(); }
	int getInstructionPointer() { return instructionPointer; }
};
*/
