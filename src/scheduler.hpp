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


