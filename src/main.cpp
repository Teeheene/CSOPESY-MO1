#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <ctime> 
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

class Process 
{
	string processName;
	int id;
	int core;
	time_t timestamp;
	bool finished;
	vector<string> instructions;
	int instructionPointer;

public:	
	void executeInstruction() {
		//implement please ><
	}
	

	bool isFinished() const { return finished; }
	string getName() const { return processName; }
	int getId() const { return id; }
	time_t getTimestamp() const { return timestamp; }
};

enum SchedulerType { FCFS, RR };

class Scheduler { 
	vector<Process> readyQueue;
	vector<Process> finished;
	SchedulerType type;
	int quantum;
	int currentTick;
	int cpuCount;

public:
	void addProcess(Process p);
	void run();
	void runFCFS();
	void runRR();
};

class MainController
{
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
					cout << "initializing processor configuration..." << endl
						 << endl;
					Config cfg;

					if (cfg.loadFile())
					{
						std::cout << "configuration loaded successfully.\n\n";
						cfg.print();
					}
					else
					{
						std::cerr << "failed to load configuration.\n\n";
					}
					initialized = true;
				}
				else if (cmd[0] == "exit")
				{
					cout << "exiting program..." << endl
						 << endl;
					running = false;
				}
				else
				{
					cout << "Please initialize first!" << endl
						 << endl;
				}
			}

			else if (initialized)
			{
				if (cmd[0] == "screen")
				{
					if (cmd.size() == 1)
					{
						cout << "Missing argument after 'screen'" << endl
							 << endl;
					}
					else if (cmd[1] == "-s")
					{
						// clear contents
						// create a new process
						//"move" to process screen (possibly call a class for it)
					}
					else if (cmd[1] == "-ls")
					{
						// list all running processes
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
					cout << "exiting program..." << endl
						 << endl;
					running = false;
				}
				else
				{
					cout << "Unknown command." << endl
						 << endl;
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
