#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include "initialize.hpp"
using namespace std;

atomic<bool> running(true);

/*
 * tokenizes the input
 *
 * @param input - the string input of the user
 * @returns vector<string> - dynamic array of tokens
 * */
std::vector<std::string> tokenizeInput(std::string input)
{
	std::vector<std::string> tokens;
	std::string token{""};

	if (input.empty())
		return {};

	for (char ch : input)
	{
		// if its not a space
		if (!std::isspace(static_cast<unsigned char>(ch)))
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
