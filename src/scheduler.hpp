struct Core {
	int id;
	atomic<bool> active;
	unique_ptr<Process> current;
	thread worker;
	mutex coreMtx;

	Core(int cid) :
		id(cid),
		active(false)
	{}
};

class Scheduler
{
	vector<unique_ptr<Core>> cores;
	vector<unique_ptr<Process>> readyQueue;
	vector<unique_ptr<Process>> finished;
	mutex mtx;

	//thread bool
	atomic<bool> stop;

	//cfg
	string type;
	int quantum;
	int execDelay;
	int cpuCount;
	
	//tick
	int cpuCycle;

public:
	//debugging
	Scheduler(int cpuCount_) : 
		cpuCount(cpuCount_),
		type("fcfs"),
		quantum(3),
		execDelay(2000),
		cpuCycle(0),
		stop(false)
	{
		cores.reserve(cpuCount);
		for(int i = 0; i < cpuCount; i++) {
			cores.emplace_back(make_unique<Core>(i));
		}
	}

	Scheduler(Config cfg) : 
		type(cfg.scheduler),
		quantum(cfg.quantumCycles),
		execDelay(cfg.delayExec),
		cpuCount(cfg.numcpu),
		cpuCycle(0),
		stop(false)
	{
		cores.reserve(cpuCount);
		for(int i = 0; i < cpuCount; i++) {
			cores.emplace_back(make_unique<Core>(i));
		}
	}

	// pushes process into ready queue
	void addProcess(unique_ptr<Process> process) {
		lock_guard<mutex> lock(mtx);
		readyQueue.push_back(process);
	};

	// removes process at the front of ready queue and returns it
	optional<unique_ptr<Process>> getNextProcess() {
		lock_guard<mutex> lock(mtx);

		//returns null ptr if queue is empty
		if (readyQueue.empty()) return nullopt;

		//removes proc from queue and returns
		auto p = move(readyQueue.front());
		readyQueue.erase(readyQueue.begin());
		return p;
	}

	void startScheduler() {
		//threads for each core
		for(auto &core : cores) {
			//thread(...) in the background it will run the instr/ worker in the bg
			//[this, &core] a lambad capt list, states which vars to use inside thread funct.
			core->worker = thread([this, &core]() {
				while(!stop) {
					//gets process from ready queue
					auto nextProc = getNextProcess();	
					if(nextProc.has_value()) {
						{
							lock_guard<mutex> lock(core->coreMtx);
							core->active = true;
							core->current = move(nextProc.value());
						}

						//for limiting instruction time
						int limit;
						{
							lock_guard<mutex> lock(core->coreMtx);

							//depending on the scheduler type;
							//rr chooses the limit between the shorter one
							//		if proc has lower instr or quantum is less
							//fcfs runs to completion
							if(type == "rr") {
								limit = min(quantum, core->current->getInstructionCount())
							} else {
								limit = core->current->getInstructionCount();
							}
						}

						//runs to limit depending on type
						for(int i = 0; i < limit && !stop; i++) {
							{
								lock_guard<mutex> lock(core->coreMtx);
								//executes the instruction and moves the pointer
								core->current->executeNextInstruction(core->id);
							}
							this_thread::sleep_for(chrono::milliseconds(freqDelay));
						}
					
						//either pushback in the queue or push into finished queue
						{
							lock_guard<mutex> lock(core->coreMtx);
							if(core->current->hasRemainingInstruction() && mode == "rr") {
								lock_guard<mutex> lock2(mtx);
								readyQueue.push_back(move(core->current));	
							} else {
								finished.push_back(move(core->current));
							}
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
	
	void simulateCpuCycle() {
		while(!stop) {
			cpuCycle++;
			this_thread::sleep_for(chrono::milliseconds(100));
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

	//debugging
	void state() {
		cout << "=============================" << endl;
		cout << "Cycle: " << cpuCycle << endl;

		for(auto &core : cores) {
			//temporary values to store threaded core
			int id;
			Process* tempProcess;
			bool isActive;

			{
				lock_guard<mutex> lock(core->coreMtx);
				id = core->id;
				tempProcess = core->current.get();
				isActive = core->active;
			}
			if(isActive) {
				cout << "[CORE " << id << "] Running Proc-" 
					<< tempProcess->getPid() << " (" 
					<< tempProcess->getInstructionPointer() << " / " 
					<< tempProcess->getInstructionCount()
					<< ")" << endl;
			} else {
				cout << "[CORE " << id << "] Idle" << endl;
			}
		}

		{
			lock_guard<mutex> lock(mtx);
			cout << "Ready Queue: " << endl;
			for(auto &proc : readyQueue) {
				cout << "[PROC-" << proc->getPid() << "] Waiting..." << endl;
			}
			cout << "Finished Queue: " << endl;
			for(auto &proc : finished) {
				cout << "[PROC-" << proc->getPid() << "] Finished Running " 
					<< proc->getInstructionCount() << " instructions." << endl;
			}
		}
		cout << "=============================" << endl;
	}

	void searchProcess(int pid) {
		lock_guard<mutex> lock(mtx);
		for(auto &core : cores) {
			lock_guard<mutex> lock(core->coreMtx);
			if(core->current && core->current->getPid() == pid) {
				cout << "PROC-" << pid << endl;
				core->current->printLogs();
			}
		}
	}
};


