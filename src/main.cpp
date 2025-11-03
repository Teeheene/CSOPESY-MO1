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
#include <random>
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
	// formulate this shit
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

enum SchedulerType
{
	FCFS,
	RR
};

class Scheduler
{
	vector<Process> readyQueue;
	vector<Process> finished;
	atomic<int> currentTick{0};
	mutex mtx;

	// configured
	SchedulerType type;
	int quantum;
	int batchFreq;
	int minIns;
	int maxIns;
	int delayExec;
	int numCores;

	thread schedulerThread;
	vector<std::thread> coreThreads;

	atomic<bool> generatingProcesses{false};
	atomic<bool> runningCores{false};
	atomic<int> nextProcessId{0};

public:
	Scheduler() : currentTick(0),
		type(SchedulerType::FCFS),
		quantum(5),
		batchFreq(1),
		minIns(1000),
		maxIns(2000),
		delayExec(0)
	{}

	Scheduler(Config cfg) : currentTick(0),
		type(cfg.scheduler == "rr" ? SchedulerType::RR : SchedulerType::FCFS),
		quantum(cfg.quantumCycles),
		batchFreq(cfg.batchFreq),
		minIns(cfg.minIns),
		maxIns(cfg.maxIns),
		delayExec(cfg.delayExec),
		numCores(cfg.numcpu)
	{}

	~Scheduler()
	{
		stopGeneration();
	}

	// pushes process into ready queue
	void addProcess(Process p)
	{
		lock_guard<mutex> lock(mtx);
		readyQueue.push_back(p);
	};

	// removes process at the front of ready queue and returns it
	optional<Process> getNextProcess()
	{
		lock_guard<mutex> lock(mtx);
		if (readyQueue.empty())
			return nullopt;
		Process p = readyQueue.front();
		readyQueue.erase(readyQueue.begin());
		return p;
	}

	// marks the process finished (more like puts it in the finished queue)
	void markFinished(Process &p)
	{
		lock_guard<mutex> lock(mtx);
		finished.push_back(p);
	}

	// checks for readyqueue if its still working
	bool hasWork()
	{
		lock_guard<mutex> lock(mtx);
		return !(readyQueue.empty());
	}

	void FCFS(int coreId)
	{
		while (true)
		{
			optional<Process> tempProcess = getNextProcess();
			if (!tempProcess.has_value())
				break;
			Process process = tempProcess.value();

			while (!process.isFinished())
			{
				// execute instruction logic
				this_thread::sleep_for(chrono::milliseconds(delayExec));
			}

			markFinished(process);
		}
	}

	void enterProcessScreen(Process p)
	{
		string rawInput;
		vector<string> cmd;

		// clear screen
		// cout << "\033[2J\033[1;1H";

		while (true)
		{
			cout << "root:\\> ";
			getline(cin, rawInput);
			cmd = tokenizeInput(rawInput);

			if (cmd[0] == "process-smi")
			{
				cout << endl;
				cout << "Process name: " << p.getName() << endl;
				cout << "ID: " << p.getId() << endl;
				cout << "Logs:" << endl;
				// implement ts ((timestamp) Core: N "instruction")
				cout << "Current instruction Line: " << p.getCurrentInstructionLine() << endl;
				cout << "Lines of code: " << p.getInstructionSize() << endl
					 << endl;
				// when finished print finished type shi
			}
			else if (cmd[0] == "exit")
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
		for (const auto &process : readyQueue)
		{
			if (process.getName() == processName)
			{
				return process;
			}
		}
		return nullopt;
	}
	void startGeneration()
	{
		if (generatingProcesses)
			return;

		generatingProcesses = true;
		runningCores = true;

		schedulerThread = std::thread(&Scheduler::run, this);

		cout << "Booting " << numCores << " CPU core(s)..." << endl;
		for (int i = 0; i < numCores; i++)
		{
			if (type == SchedulerType::FCFS)
			{
				coreThreads.emplace_back(&Scheduler::FCFS, this, i);
			}
			else if (type == SchedulerType::RR)
			{
				// implement RR
			}
		}
	}

	void stopGeneration()
	{
		generatingProcesses = false;
		runningCores = false;

		if (schedulerThread.joinable())
		{
			schedulerThread.join();
		}

		for (auto &t : coreThreads)
		{
			if (t.joinable())
			{
				t.join();
			}
		}
		coreThreads.clear();
	}

	void run()
	{
		while (generatingProcesses)
		{
			currentTick++;

			if (currentTick % batchFreq == 0)
			{
				int id = nextProcessId++;
				string processName = "p" + to_string(id);

				Process newProcess(processName, 0, minIns, maxIns);

				addProcess(newProcess);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
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
						// initializes the scheduler
						scheduler = make_unique<Scheduler>(cfg);
						initialized = true;
					}
					else
					{
						std::cerr << "failed to load configuration.\n\n";
					}
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
						if (cmd.size() == 2)
						{
							//to implement
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
						if (cmd.size() == 2)
						{
							cout << "Missing argument: Process Name" << endl;
						}
						else
						{
							optional<Process> pTemp = scheduler->searchProcess(cmd[2]);
							if (pTemp)
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
						for (int i = 0; i <= 38; i++)
						{
							cout << "-";
						}
						cout << endl;
						cout << "Running processes: " << endl;
						// call scheduler function to show all running
						cout << endl;
						cout << "Finished processes: " << endl;
						// call scheduler function to show all finished
						for (int i = 0; i <= 38; i++)
						{
							cout << "-";
						}
						cout << endl
							 << endl;
					}
				}
				else if (cmd[0] == "scheduler-start" || cmd[0] == "scheduler-test")
				{
					scheduler->startGeneration();
				}
				else if (cmd[0] == "scheduler-stop")
				{
					scheduler->stopGeneration();
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
