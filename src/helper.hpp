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

//basic random process generator 
unique_ptr<Process> createRandomProcess(int pid, int minIns, int maxIns) {
	static vector<string> ops = {"LOAD", "ADD", "SUB", "DECLARE"};
	auto p = make_unique<Process>(pid);
	
	//generates instructions depending on cfg
	srand(time(nullptr) + pid);
	int len = rand() % (maxIns - minIns + 1) + min; 

	for(int i = 0; i < len; i++) {
		/*Instruction ins = generateRandomInstruction();
		if(ins.isForLoop()) {
			vector<Instruction> processedForLoop = processForLoop(ins);
			for(auto &subIns : processedForLoop) {
				p->addInstruction(subIns);
			}
		}
		else
			p->addInstruction(ins);
		*/

		//temporary
		int index = rand() % ops.size();
		p->addInstruction(ops[index]);

	}

	//returns the newly made process
	return p;
}
