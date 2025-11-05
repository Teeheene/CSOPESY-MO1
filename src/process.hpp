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

	string toStringTimestamp() {
		//get timestamp adjusted to local time
		char strTime[100];
		tm* translTimestamp = localtime(&timestamp);
		strftime(strTime, sizeof(strTime), "%m/%d/%Y %I:%M:%S%p", translTimestamp);

		//print the log details
		return "(" + string(strTime) + ")";
	}

	string toString() {
		//get timestamp adjusted to local time
		char strTime[100];
		tm* translTimestamp = localtime(&timestamp);
		strftime(strTime, sizeof(strTime), "%m/%d/%Y %I:%M:%S%p", translTimestamp);

		//print the log details
		return "(" + string(strTime) + ")" + " Core:" + to_string(core)
			+ " \"" + instr + "\"\n";
	}
};

class Process {
	string name;
	int pid;
	int instructionPointer;
	int sleepTimer;
	map<string, int> memory;
	vector<Instruction> instructions;
	vector<unique_ptr<Log>> logs;
	mutex logMtx;

public:
	Process(int pid_, string name_) :
		pid(pid),
		name(name_),
		instructionPointer(0),
		sleepTimer(0)
	{}

	Process() :
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

	int executeNextInstruction(int core) {
		if(!hasRemainingInstructions()) { return 0; }

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
				logs.push_back(make_unique<Log>(core, instr.getOutput()));
        }
	
			else if (op == "SLEEP") { 
      		if (args.size() >= 1) {
               sleepTimer = getValue(args[0]);
					instructionPointer++;
					return sleepTimer;
   			}
			}
        
		instructionPointer++;
		return 0;
	}

	void decSleepTimer() {
		sleepTimer--;
	}

	int getSleepTimer() {
		return sleepTimer;
	}

	void printLogs() {
		lock_guard<mutex> lock(logMtx);
		cout << "ID: " << pid << endl;
		cout << "Logs:" << endl;
		for(const auto& log : logs) {
			log->print();
		}
	}

	string toStringRecentTimeLog() {
		lock_guard<mutex> lock(logMtx);
		return logs.back()->toStringTimestamp();
	}

	string toStringLogs() {
		lock_guard<mutex> lock(logMtx);
		string result;
		for(const auto& log : logs) {
			result += log->toString();
		}
		return result;
	}

	string getName() { return name; }
	int getPid() { return pid; }
	int getInstructionCount() { return instructions.size(); }
	int getInstructionPointer() { return instructionPointer; }
	bool isAsleep() { return sleepTimer > 0; }
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
