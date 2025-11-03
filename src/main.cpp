#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <ctime> 
#include <optional>
#include <unordered_map>
#include "initialize.hpp"
using namespace std;

atomic<bool> running(true);

/*
 * tokenizes the input
 *
 * @param input - the string input of the user
 * @returns vector<string> - dynamic array of tokens
 * */
vector<string> tokenizeInput(string input)
{
	vector<std::string> tokens;
	string token{""};

	if (input.empty())
		return {};

	for (char ch : input)
	{
		// if its not a space
		if (!isspace(static_cast<unsigned char>(ch)))
		{
			token += ch;
		}
		else
		{
			tokens.push_back(token);
			token = "";
		}
	}
	tokens.push_back(token);

	return tokens;
}

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

struct Log 
{
	time_t timestamp;
	int instructionLine;
	string display;
	//formulate this shit
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
	Process(string name, int assignedCore) :
		processName(name),
		core(assignedCore),
		id(nextId++),
		finished(false),
		instructionPointer(0)
	{}

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
	uint16_t getValue(std::string &token)
	{
		if (vars.count(token))
			return vars[token].value;
		return static_cast<uint16_t>(std::stoi(token));
	}
};
int Process::nextId = 0;


enum SchedulerType { FCFS, RR };

class Scheduler 
{ 
	vector<Process> readyQueue;
	vector<Process> finished;
	int currentTick;
	mutex mtx;

	//configured
	SchedulerType type;
	int quantum;
	int batchFreq;
	int minIns;
	int maxIns;
	int delayExec;

public:
	Scheduler() :
		currentTick(0),
		type(SchedulerType::FCFS),
		quantum(5),
		batchFreq(1),
		minIns(1000),
		maxIns(2000),
		delayExec(0)
	{}

	Scheduler(Config cfg) :
		currentTick(0),
		type(cfg.scheduler == "rr" ? SchedulerType::RR : SchedulerType::FCFS),
		quantum(cfg.quantumCycles),
		batchFreq(cfg.batchFreq),
		minIns(cfg.minIns),
		maxIns(cfg.maxIns),
		delayExec(cfg.delayExec)
	{}

	//pushes process into ready queue
	void addProcess(Process p) 
	{
		lock_guard<mutex> lock(mtx);
		readyQueue.push_back(p);
	}; 

	//removes process at the front of ready queue and returns it
	optional<Process> getNextProcess() {
		lock_guard<mutex> lock(mtx);
		if(readyQueue.empty()) return nullopt;
		Process p = readyQueue.front();
		readyQueue.erase(readyQueue.begin());
		return p;
	}

	//marks the process finished (more like puts it in the finished queue)
	void markFinished(Process &p) {
		lock_guard<mutex> lock(mtx);
		finished.push_back(p);
	}

	//checks for readyqueue if its still working
	bool hasWork()
	{
		lock_guard<mutex> lock(mtx);
		return !(readyQueue.empty());
	}

	void FCFS(int coreId) 
	{
		while(true)
		{
			optional<Process> tempProcess = getNextProcess();
			if(!tempProcess.has_value()) break;
			Process process = tempProcess.value();

			while(!process.isFinished())
			{
				//execute instruction logic
				
				this_thread::sleep_for(chrono::milliseconds(delayExec));
			}

			markFinished(process);
		}
	}

	void enterProcessScreen(Process p) 
	{
		string rawInput;
		vector<string> cmd;
		
		//clear screen
		//cout << "\033[2J\033[1;1H";

		while(true) {
			cout << "root:\\> ";
			getline(cin, rawInput);
			cmd = tokenizeInput(rawInput);

			if(cmd[0] == "process-smi") 
			{
				cout << endl;
				cout << "Process name: " << p.getName() << endl;
				cout << "ID: " << p.getId() << endl;
				cout << "Logs:" << endl;
				//implement ts ((timestamp) Core: N "instruction")
				cout << "Current instruction Line: " << p.getCurrentInstructionLine() << endl;
				cout << "Lines of code: " << p.getInstructionSize() << endl <<
					endl;
				//when finished print finished type shi
			}
			else if(cmd[0] == "exit") 
			{
				cout << "Returning home..." << endl;
				break;
			}
			else 
			{
				cout << "Unknon command inside process screen." << endl;
			}
		}
	}
	optional<Process> searchProcess(string processName) 
	{
		for(const auto &process : readyQueue) 
		{
			if(process.getName() == processName)
			{
				return process;
			}
		}	
		return nullopt;
	}
	void run()
	{
		while(true) 
		{
			currentTick++;
		}
	};
	void runFCFS() 
	{

	};
	void runRR();
};

class MainController
{
	unique_ptr<Scheduler> scheduler;
	string rawInput;
	vector<string> cmd;
	bool initialized;

public:
	MainController()
	{
		initialized = false;
	}

	void run()
	{
		while (running)
		{
			cout << "root:\\> ";
			getline(cin, rawInput);
			cmd = tokenizeInput(rawInput);

			if (!initialized)
			{
				if (cmd[0] == "initialize")
				{
					Config cfg;
					cout << "initializing processor configuration..." << endl;

					if (cfg.loadFile())
					{
						std::cout << "configuration loaded successfully.\n\n";
						cfg.print();
						//initializes the scheduler
						scheduler = make_unique<Scheduler>(cfg);
					}
					else
					{
						std::cerr << "failed to load configuration.\n\n";
					}
					initialized = true;
				}
				else if (cmd[0] == "exit")
				{
					cout << "exiting program..." << endl;
					running = false;
				}
				else
				{
					cout << "Please initialize first!" << endl;
				}
			}

			else if (initialized)
			{
				if (cmd[0] == "screen")
				{
					if (cmd.size() == 1)
					{
						cout << "Missing argument after 'screen'" << endl;
					}
					else if (cmd[1] == "-s")
					{
						if(cmd.size() == 2) 
						{
							cout << "Missing argument: Process Name" << endl;
						}
						else
						{
							Process pTemp(cmd[2], 0);
							scheduler->addProcess(pTemp);
							scheduler->enterProcessScreen(pTemp);
						}
					}
					else if (cmd[1] == "-r")
					{
						if(cmd.size() == 2)
						{
							cout << "Missing argument: Process Name" << endl;
						}
						else
						{
							optional<Process> pTemp = scheduler->searchProcess(cmd[2]);
							if(pTemp)
								scheduler->enterProcessScreen(*pTemp);
							else
								cout << "Process <" << cmd[2] << "> not found." << endl;
						}
					}
					else if (cmd[1] == "-ls")
					{
						cout << "CPU utilization: " << endl;
						cout << "Cores used: " << endl;
						cout << "Cores available: " << endl 
							<< endl;
						for(int i = 0; i <= 38; i++) { cout << "-"; }
						cout << endl;
						cout << "Running processes: " << endl;
						//call scheduler function to show all running
						cout << endl;
						cout << "Finished processes: " << endl; 
						//call scheduler function to show all finished
						for(int i = 0; i <= 38; i++) { cout << "-"; }
						cout << endl << endl;
					}
				}
				else if (cmd[0] == "scheduler-start")
				{
					// activate the thread for generating processes in the background
				}
				else if (cmd[0] == "scheduler-stop")
				{
					// stop generating dummy processes
				}
				else if (cmd[0] == "report-util")
				{
					// generate CPU util report
				}
				else if (cmd[0] == "exit")
				{
					cout << "exiting program..." << endl;
					running = false;
				}
				else
				{
					cout << "Unknown command." << endl;
				}
			}
		}
	}
};

int main()
{
	MainController os;
	os.run();
}
