class MainController
{
	string rawInput;
	vector<string> cmd;
	bool initialized;

public:
	MainController() :
		initialized(false)
	{}

	void run()
	{	
		Scheduler scheduler;
		thread t;
		int pid = 0;
		int minIns = 0;
		int maxIns = 0;

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
						scheduler.configure(cfg);
						scheduler.start();
						//initialize instructions
						minIns = cfg.minIns;
						maxIns = cfg.maxIns;
						std::cout << "scheduler started successfully.\n\n";
						initialized = true;
						
						t = thread([&scheduler]() { scheduler.simulate(); });
						t.detach();
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
							scheduler.addProcess(createRandomProcess(pid++));
							//to implement
						}
						else
						{
							scheduler.addProcess(createRandomProcess(pid++));
							//scheduler->enterProcessScreen(pTemp);
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
							/*if(scheduler.processExists(cmd[2]))
								cout << "tbi" << endl;
								//scheduler->enterProcessScreen(*pTemp);
							else
								cout << "Process <" << cmd[2] << "> not found." << endl;
								*/
						}
					}
					else if (cmd[1] == "-ls") {
						scheduler.state();
					}
				}
				else if (cmd[0] == "scheduler-start" || cmd[0] == "scheduler-test")
				{
					//scheduler.startTest();
				}
				else if (cmd[0] == "scheduler-stop")
				{
					//scheduler.stopTest();
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


