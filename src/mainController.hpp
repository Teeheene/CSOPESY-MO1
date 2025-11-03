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


