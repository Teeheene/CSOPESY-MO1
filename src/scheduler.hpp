
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

class Scheduler {
	vector<unique_ptr<Core>> cores;	
	vector<unique_ptr<Process>> readyQueue;
	vector<unique_ptr<Process>> finished;
	vector<unique_ptr<Process>> sleepingQueue;
	mutex mtx;

	//cfg
	int coreCount;
	string mode;
	int quantum;
	int cpuCycle;
	int execDelay;
	int batchFreq;
	int minIns;
	int maxIns;
	
	//
	int freq;
	atomic<bool> stop;
	atomic<bool> test;

public:
	Scheduler() :
		coreCount(0),
		mode("fcfs"),
		quantum(3),
		execDelay(10),
		cpuCycle(0),
		batchFreq(10),
		minIns(5),
		maxIns(10),
		freq(0),
		stop(false),
		test(false)
	{}

	void configure(Config cfg) {
		mode = cfg.scheduler;
		quantum = cfg.quantumCycles;
		execDelay = cfg.delayExec;
		coreCount = cfg.numcpu;
		batchFreq = cfg.batchFreq; 
		minIns = cfg.minIns;
		maxIns = cfg.maxIns;

		cores.reserve(coreCount);
		for(int i = 0; i < coreCount; i++) {
			cores.emplace_back(make_unique<Core>(i));
		}
	}

	void addProcess(unique_ptr<Process> p) {
		lock_guard<mutex> lock(mtx);
		readyQueue.push_back(move(p));
	}

	optional<unique_ptr<Process>> getNextProcess() {
		lock_guard<mutex> lock(mtx);
		if(readyQueue.empty()) return nullopt;

		auto p = move(readyQueue.front());
		readyQueue.erase(readyQueue.begin());
		return p;
	}

	void start() {
		//threads for each core
		for(auto &core : cores) {
			//thread(...) in the background it will run the instr/ worker in the bg
			//[this, &core] a lambad capt list, states which vars to use inside thread funct.
			core->worker = thread([this, &core]() {
				while(!stop) {
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
							limit = (mode == "rr") 
								? min(quantum, core->current->getInstructionCount()) 
								: core->current->getInstructionCount();
						}
						
						//fcfs implementation (this runs to completion) 
						for(int i = 0; i < limit && !stop && mode == "fcfs"; i++) { 
							{
								lock_guard<mutex> lock(core->coreMtx); 
								if(core->current->executeNextInstruction(core->id)) { 
									i--; 
								}; 
							} 
							this_thread::sleep_for(chrono::milliseconds(execDelay)); 
						}							

						for(int i = 0; i < limit && !stop && mode == "rr"; i++) {
							{
								lock_guard<mutex> lock(core->coreMtx);
								if(core->current->executeNextInstruction(core->id)) {
									break;
								} 
							}
							this_thread::sleep_for(chrono::milliseconds(execDelay));
						}

						{
							lock_guard<mutex> lock(core->coreMtx);
							lock_guard<mutex> lock2(mtx);
							if(core->current->isAsleep())
								sleepingQueue.push_back(move(core->current));	
							else if(core->current->hasRemainingInstructions())
								readyQueue.push_back(move(core->current));
							else
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

	void state() {
		struct CoreSnapshot{
			//core snap
			int id;
			bool active;
			//proc snap
			string name;
			int instrPointer;
			int instrCount;
			string logs;
		};

		int activeCount = 0;
		vector<CoreSnapshot> coreSnapshot;

		//snapshots the cores/ running processes
		for(auto &core : cores) {
			//temporary values to store threaded core
			int id = -1;
			bool isActive = false;
			Process* proc;
			{
				lock_guard<mutex> lock(core->coreMtx);
				id = core->id;
				isActive = core->active;
				if(isActive && core->current)
					proc = core->current.get();
			}
			if(core->active) {
				coreSnapshot.push_back({
						core->id, 
						true, 
						proc->getName(),
						proc->getInstructionPointer(),
						proc->getInstructionCount(),
						proc->toStringRecentTimeLog()});
				activeCount++;
			}
		}

		cout << "CPU utilization: " << (1.0*activeCount/coreCount*100) << "%" << endl;
		cout << "Cores used: " << activeCount << endl;
		cout << "Cores available: " << (coreCount - activeCount) << endl
			<< endl;
		
		for (int i = 0; i <= 38; i++) { cout << "-"; }
		cout << endl;
		//running processes
		cout << "Running processes:" << endl;
		for(auto &core : coreSnapshot) {
			cout << core.name << "\t" 
				<< core.logs << "\tCore: " 
				<< core.id << "\t"
				<< core.instrPointer << " / " 
				<< core.instrCount << endl;
		}

		//finished processes
		cout << "Finished processes: " << endl;
		{
			lock_guard<mutex> lock(mtx);
			for(auto &proc : finished) {
				cout << proc->getName() << "\t"
					<< proc->toStringRecentTimeLog() << "\tFinished\t" 
					<< proc->getInstructionPointer() << " / "
					<< proc->getInstructionCount() << endl;

			}
		}
		for (int i = 0; i <= 38; i++) { cout << "-"; }
		cout << endl;
	}

	string reportUtil() {
		
		stringstream reportStream;

		struct CoreSnapshot{
			int id;
			bool active;
			string name;
			int instrPointer;
			int instrCount;
			string logs;
		};

		int activeCount = 0;
		vector<CoreSnapshot> coreSnapshot;

		for(auto &core : cores) {
			int id = -1;
			bool isActive = false;
			Process* proc = nullptr; 
			{
				lock_guard<mutex> lock(core->coreMtx);
				id = core->id;
				isActive = core->active;
				if(isActive && core->current)
					proc = core->current.get();
			}
			
			if(isActive) { 
				if (proc) {
					coreSnapshot.push_back({
							id, 
							true, 
							proc->getName(),
							proc->getInstructionPointer(),
							proc->getInstructionCount(),
							proc->toStringRecentTimeLog()});
					activeCount++;
				}
			}
		}

		reportStream << "CPU utilization: " << (1.0*activeCount/coreCount*100) << "%" << endl;
		reportStream << "Cores used: " << activeCount << endl;
		reportStream << "Cores available: " << (coreCount - activeCount) << endl
					<< endl;
		
		for (int i = 0; i <= 38; i++) { reportStream << "-"; }
		reportStream << endl;
		
		reportStream << "Running processes:" << endl;
		for(auto &core : coreSnapshot) {
			reportStream << core.name << "\t" 
						<< core.logs << "\tCore: " 
						<< core.id << "\t"
						<< core.instrPointer << " / " 
						<< core.instrCount << endl;
		}

		reportStream << "Finished processes: " << endl;
		{
			lock_guard<mutex> lock(mtx);
			for(auto &proc : finished) {
				reportStream << proc->getName() << "\t"
							<< proc->toStringRecentTimeLog() << "\tFinished\t" 
							<< proc->getInstructionPointer() << " / "
							<< proc->getInstructionCount() << endl;

			}
		}
		for (int i = 0; i <= 38; i++) { reportStream << "-"; }
		reportStream << endl;

		return reportStream.str();
	}

	void tick() {
		lock_guard<mutex> lock(mtx);
		for (int i = 0; i < sleepingQueue.size(); i++) {
			Process* p = sleepingQueue[i].get();
			p->decSleepTimer();

			if (p->getSleepTimer() <= 0) {
				readyQueue.push_back(move(sleepingQueue[i]));
				sleepingQueue.erase(sleepingQueue.begin() + i); // safe erase
			} else {
				i++; // only increment if we didn’t erase
			}
		}
	}

	void simulate() {
		while(!stop) {
			tick();
			cpuCycle++;
			if(test) {
				freq++;	
				if(freq >= batchFreq) {
					lock_guard<mutex> lock(mtx);
					unique_ptr<Process> proc = createRandomProcess();
					readyQueue.push_back(move(proc));
					freq = 0;
				}
			}
			this_thread::sleep_for(chrono::milliseconds(100));
		}
	}

	void startTest() {
		freq = 0;
		test = true;
		cout << "Test has started..." << endl;
	}

	void stopTest() {
		test = false;
	}

	optional<Process*> searchProcess(string name) {
		for(auto &core : cores) {
			lock_guard<mutex> lock(core->coreMtx);
			if(core->current && core->current->getName() == name) {
				return core->current.get();
			}
		}

		{
			lock_guard<mutex> lock(mtx);
			for(auto &proc : readyQueue) {
				if(proc && proc->getName() == name) {
					return proc.get();
				}	
			}

			for(auto &proc : finished) {
				if(proc && proc->getName() == name) {
					return proc.get();
				}	
			}

			for(auto &proc : sleepingQueue) {
				if(proc && proc->getName() == name) {
					return proc.get();
				}
			}
		}

		return nullopt;
	}

	void enterProcessScreen(string name)
	{
		string rawInput;
		vector<string> cmd;
		bool screenDisplay = true;

		auto rawProc = searchProcess(name);

		if(!rawProc) {
			cout << "Process <" << name << "> not found." << endl;
			screenDisplay = false;
		} else {
			//clear screen
			cout << "\033[2J\033[1;1H";
		}

		Process *p = *rawProc;
		while (screenDisplay)
		{
			cout << "root:\\> ";
			getline(cin, rawInput);
			cmd = tokenizeInput(rawInput);

			if (cmd[0] == "process-smi")
			{
				cout << endl;
				cout << "Process name: " << p->getName() << endl;
				cout << "ID: " << p->getPid() << endl;
				cout << "Logs:" << endl << p->toStringLogs() << endl;
				if(p->getInstructionPointer() != p->getInstructionCount()) {
					cout << "Current instruction Line: " << p->getInstructionPointer() << endl;
					cout << "Lines of code: " << p->getInstructionCount() << endl
						<< endl;
				} else {
					cout << "Finished!" << endl
						<< endl;
				}
			}
			else if (cmd[0] == "exit")
			{
				cout << "Returning home..." << endl;
				break;
			}
			else
			{
				cout << "Unknown command inside process screen." << endl;
			}
		}
	}

};



/*
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
	int batchFreq; 
	int freq;
	int minIns;
	int maxIns;
	
	//tick
	int cpuCycle;
	atomic<bool> test;

public:
	//debugging
	Scheduler() : 
		cpuCount(0),
		type("fcfs"),
		quantum(3),
		execDelay(2000),
		batchFreq(10000),
		freq(0),
		cpuCycle(0),
		stop(false),
		test(false)
	{}

	void configure(Config cfg) {
		type = cfg.scheduler;
		quantum = cfg.quantumCycles;
		execDelay = cfg.delayExec;
		cpuCount = cfg.numcpu;
		batchFreq = cfg.batchFreq;
		minIns = cfg.minIns;
		maxIns = cfg.maxIns;

		cores.reserve(cpuCount);
		for(int i = 0; i < cpuCount; i++) {
			cores.emplace_back(make_unique<Core>(i));
		}
	}

	// pushes process into ready queue
	void addProcess(unique_ptr<Process> process) {
		lock_guard<mutex> lock(mtx);
		readyQueue.push_back(move(process));
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
							limit = (type == "rr") 
								? min(quantum, core->current->getInstructionCount()) 
								: core->current->getInstructionCount();
						}

						//fcfs implementation (this runs to completion)
						for(int i = 0; i < limit && !stop; i++) {
							{
								lock_guard<mutex> lock(core->coreMtx);
								core->current->executeNextInstruction(core->id);
							}
							this_thread::sleep_for(chrono::milliseconds(execDelay));
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


	/ *
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
								limit = min(quantum, core->current->getInstructionCount());
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
							this_thread::sleep_for(chrono::milliseconds(execDelay));
						}

						unique_ptr<Process> procToMove;
						bool shouldRequeue;
						{
							lock_guard<mutex> lock(core->coreMtx);
							shouldRequeue = (core->current->hasRemainingInstructions() && type == "rr");
							procToMove = move(core->current);
							core->active = false;
						}
					
						//either pushback in the queue or push into finished queue
						if (procToMove) {
							lock_guard<mutex> lock(mtx);
							if(shouldRequeue) {
								readyQueue.push_back(move(procToMove));	
							} else {
								finished.push_back(move(procToMove));
							}
						}
					} else {
						this_thread::sleep_for(chrono::milliseconds(50));
					}
				} 
			});
			//end of thread
		}
	}
	* /

	void stopScheduler() {
		stop = true;
		for(auto &core : cores) {
			if(core->worker.joinable())
				core->worker.join();
		}
		cout << "All cores stopped." << endl;
	}
	
	void simulate() {
		while(!stop) {
			cpuCycle++;
			if(test) {
				freq++;	
				if(batchFreq % freq == 0) {
					lock_guard<mutex> lock(mtx);
					int id = readyQueue.back()->getPid() + 1;
					addProcess(createRandomProcess(id, minIns, maxIns));
				}
			}
			this_thread::sleep_for(chrono::milliseconds(100));
		}
	}

	void startTest() {
		freq = 0;
		test = true;
	}

	void stopTest() {
		test = false;
	}

	//debugging
	/ *
	void state() {
		struct CoreSnapshot{
			//core snap
			int id;
			bool active;
			//proc snap
			string name;
			int instrPointer;
			int instrCount;
			string logs;
		};

		int activeCount = 0;
		vector<CoreSnapshot> coreSnapshot;

		//snapshots the cores/ running processes
		for(auto &core : cores) {
			//temporary values to store threaded core
			int id = -1;
			bool isActive = false;
			Process* proc;
			{
				lock_guard<mutex> lock(core->coreMtx);
				id = core->id;
				isActive = core->active;
				if(isActive && core->current)
					proc = core->current.get();
			}
			if(core->active) {
				coreSnapshot.push_back({
						core->id, 
						true, 
						proc->getName(),
						proc->getInstructionPointer(),
						proc->getInstructionCount(),
						proc->toStringLogs()});
				activeCount++;
			}
		}

		cout << "CPU utilization: " << (activeCount/cpuCount*100) << "%" << endl;
		cout << "Cores used: " << activeCount << endl;
		cout << "Cores available: " << (cpuCount - activeCount) << endl
			<< endl;
		
		for (int i = 0; i <= 38; i++) { cout << "-"; }
		cout << endl;
		//running processes
		cout << "Running processes:" << endl;
		for(auto &core : coreSnapshot) {
			cout << core.name << "\t";
			core.logs;
			cout << "\tCore: " << core.id << "\t"
				<< core.instrPointer << " / " 
				<< core.instrCount << endl;
		}

		//finished processes
		cout << "Finished processes: " << endl;
		{
			lock_guard<mutex> lock(mtx);
			for(auto &proc : finished) {
				cout << proc->getName() << "\t";
				proc->printLogs();
				cout << "\tFinished\t" 
					<< proc->getInstructionPointer() << " / "
					<< proc->getInstructionCount() << endl;

			}
		}
		for (int i = 0; i <= 38; i++) { cout << "-"; }
		cout << endl;
	}
	* /
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
				isActive = core->active;
				tempProcess = core->current.get();
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


	bool processExists(string name) {
		//search core/ running processes
		for(auto &core : cores) {
			{
				lock_guard<mutex> lock(core->coreMtx);
				if(core->current->getName() == name) return true;
			}
		}
		
		//search ready and finished queue
		{
			lock_guard<mutex> lock(mtx);
			for(auto &proc : readyQueue) {
				if(proc->getName() == name) return true;
			}

			for(auto &proc : finished) {
				if(proc->getName() == name) return true;
			}
		}

		//if not found
		return false;
	}

	/ *
	//processor screen
	void enterProcessScreen(int pid)
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
				//fix ts
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
				cout << "Unknown command inside process screen." << endl;
			}
		}
	}
	* /
};*/



