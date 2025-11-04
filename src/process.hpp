#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

#include <string>
#include <optional>
#include <vector>

//randomizer stuff
#include <ctime>
#include <cstdlib>

#include "instruction.hpp"

using namespace std;

struct Log {
	int instructionPointer;
	int instructionSize;
	int core;
	string instructionString;
};

class Process {
	int pid;
	int instructionPointer;
	vector<Instruction> instructions;
	vector<Log> logs;

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
		if(!hasRemainingInstructions()) { return; }

		//log the current instruction
		Log entry{instructionPointer, static_cast<int>(instructions.size()), 
				core, instructions[instructionPointer]};
		logs.push_back(entry);

		//move the pointer forward
		instructionPointer++;
	}
	
	int getPid() { return pid; }
	int getInstructionCount() { return instructions.size(); }
	int getInstructionPointer() { return instructionPointer; }
	string getProgress() { 
		if(logs.empty())
			return "0 / " + to_string(instructions.size());
		return logs.back().instructionPointer + "/ " + logs.back().instructionSize; 
	}
	int getCore() {
		if(logs.empty())
			return -1;
		return logs.back().core;
	}
};

//basic random process generator 
Process createRandomProcess(int pid) {
	static vector<string> ops = {"LOAD", "ADD", "SUB", "DECLARE"};
	Process p(pid);

	srand(time(nullptr) + pid);
	int len = rand() % 100 + 5; //about 5-14 instructions

	for(int i = 0; i < len; i++) {
		int index = rand() % ops.size();
		p.addInstruction(ops[index]);
	}

	//returns the newly made process
	return p;
}

struct Core {
	int id;
	atomic<bool> active;
	Process current;
	thread worker;
	mutex coreMtx;

	Core(int cid) :
		id(cid),
		active(false)
	{}
};

class Scheduler {
	vector<unique_ptr<Core>> cores;	
	vector<Process> readyQueue;
	vector<Process> finished;
	mutex mtx;

	//cfg
	int coreCount;
	string mode;
	int quantum;
	int cpuCycle;
	int freqDelay;
	atomic<bool> stop;

public:
	Scheduler(int cCount, string m = "fcfs", int q = 3, int delay = 1000) :
		coreCount(cCount),
		mode(m),
		quantum(q),
		freqDelay(delay),
		cpuCycle(0),
		stop(false)
	{
		cores.reserve(cCount);
		for(int i = 0; i < coreCount; i++)
			cores.emplace_back(make_unique<Core>(i)); //similar to push_back funct of vector
	}

	void addProcess(Process p) {
		lock_guard<mutex> lock(mtx);
		readyQueue.push_back(move(p));
	}

	optional<Process> getNextProcess() {
		lock_guard<mutex> lock(mtx);
		if(readyQueue.empty()) return nullopt;

		Process p = move(readyQueue.front());
		readyQueue.erase(readyQueue.begin());
		return p;
	}

	void printFinished() {
		cout << "Finished!" << endl;
		for(auto &p : finished) {
			cout << "PID " << p.getPid() << " executed " << p.getProgress();
		}
	}

	void start() {
		int task = 0;

		//threads for each core
		for(auto &core : cores) {
			//thread(...) in the background it will run the instr/ worker in the bg
			//[this, &core] a lambad capt list, states which vars to use inside thread funct.
			core->worker = thread([this, &core, &task]() {
				while(!stop) {
					auto nextProc = getNextProcess();	
					if(nextProc.has_value()) {
						{
							lock_guard<mutex> lock(core->coreMtx);
							core->active = true;
							core->current = move(nextProc.value());
						}

						//for limiting instruction time
						int limit = (mode == "rr") 
							? min(quantum, core->current.getInstructionCount()) 
							: core->current.getInstructionCount();

						//fcfs implementation (this runs to completion)
						for(int i = 0; i < limit && !stop; i++) {
							{
								lock_guard<mutex> lock(core->coreMtx);
								core->current.executeNextInstruction(core->id);
							}
							this_thread::sleep_for(chrono::milliseconds(freqDelay));
						}
						
						//rr implementation later

						{
							lock_guard<mutex> lock(core->coreMtx);
							lock_guard<mutex> lock2(mtx);
							finished.push_back(move(core->current));
						}

						core->active = false;
					} else {
						this_thread::sleep_for(chrono::milliseconds(50));
					}
				} 
			});
			//end of thread
		}
	}

	void stopScheduler() {
		stop = true;
		for(auto &core : cores) {
			if(core->worker.joinable())
				core->worker.join();
		}
		cout << "All cores stopped." << endl;
	}

	//debugging
	void state() {
		cout << "=============================" << endl;
		cout << "Cycle: " << cpuCycle << endl;

		for(auto &core : cores) {
			//temporary values to store threaded core
			int id;
			Process tempProcess;
			bool isActive;

			{
				lock_guard<mutex> lock(core->coreMtx);
				id = core->id;
				tempProcess = core->current;
				isActive = core->active;
			}
			if(isActive) {
				cout << "[CORE " << id << "] Running Proc-" 
					<< tempProcess.getPid() << " (" << tempProcess.getInstructionPointer() 
					<< " / " << tempProcess.getInstructionCount()
					<< ")" << endl;
			} else {
				cout << "[CORE " << id << "] Idle" << endl;
			}
		}

		cout << "Ready Queue: " << endl;
		for(auto &proc : readyQueue) {
			cout << "[PROC-" << proc.getPid() << "] Running... ("
				<< proc.getProgress() << ")" << endl;
		}
		cout << "Finished Queue: " << endl;
		for(auto &proc : finished) {
			cout << "[PROC-" << proc.getPid() << "] Finished Running " 
				<< proc.getInstructionCount() << " instructions. (Core " 
				<< proc.getCore() << ")" << endl;
		}
		cout << "=============================" << endl;
	}

	void simulate() {
		while(!stop) {
			cpuCycle++;
			this_thread::sleep_for(chrono::milliseconds(100));
		}
	}
};


int main() {
	Scheduler scheduler(4);
	string cmd;
	int pid = 0;

	thread t([&scheduler]() { scheduler.simulate(); });

	while (true) {
		cout << "root:\\> ";
		getline(cin, cmd);

		if (cmd == "scheduler-start") {
			scheduler.start();
			this_thread::sleep_for(chrono::milliseconds(500)); // give cores time to start
			scheduler.state();
		}
		else if (cmd == "create-process") scheduler.addProcess(createRandomProcess(pid++));
		else if (cmd == "process-smi") scheduler.state();
		else if (cmd == "exit") {
			scheduler.stopScheduler();
			break;
		}
 	}
}