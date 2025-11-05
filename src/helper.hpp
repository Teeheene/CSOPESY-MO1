#include "globals.hpp"
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

vector<Instruction> processForLoop(int loopCount) {
   vector<Instruction> loopInstructions;

   loopInstructions.push_back(Instruction("FOR", {"ADD, PRINT"}));
	
   vector<Instruction> loopBody;
   loopBody.push_back(Instruction("ADD", {"VAR1", "VAR1", "1"}));
   loopBody.push_back(Instruction("PRINT",{"Hello World!"}));

	for (int i = 0; i < loopCount; i++) {
   	for (const auto& instr : loopBody) {
			loopInstructions.push_back(instr);
      }
   }

   return loopInstructions;
}

vector<Instruction> generateRandomInstruction() {
	static vector<string> ops = {"DECLARE", "ADD", "SUBTRACT", "PRINT", "SLEEP", "FOR"};
   string op = ops[rand() % ops.size()];
   vector<string> args;

   string var1 = "VAR" + to_string(rand() % 3);
   string var2 = "VAR" + to_string(rand() % 3);
   string var3 = "VAR" + to_string(rand() % 3);
   string literal = to_string(rand() % 50 + 1); 

	vector<Instruction> instructions;

   if (op == "DECLARE") {
		args.push_back(var1);
      args.push_back(literal); 
	} else if (op == "ADD" || op == "SUBTRACT") {
   	args.push_back(var1); 
      args.push_back(var2); 
      args.push_back((rand() % 2 == 0) ? var3 : literal); 
   } else if (op == "PRINT") {
      args.push_back("Hello World!");
   } else if (op == "SLEEP") {
      args.push_back(to_string(rand() % 3 + 1)); 
   } else if (op == "FOR") {
		int loopCount = rand() % 3 + 1;
		instructions = processForLoop(loopCount);
		return instructions;
	}

	Instruction instr = Instruction(op, args);
	instructions.push_back(instr);
	return instructions;
}


//basic random process generator 
unique_ptr<Process> createRandomProcess(string name = "PROC-") {
	//setup name and id
	int pid = nextId.fetch_add(1);
	if(name == "PROC-")
		name += to_string(pid);
	auto p = make_unique<Process>(pid, name);

	srand(time(nullptr) + pid);
	int len = rand() % (maxIns - minIns + 1) + minIns; 

	for(int i = 0; i < len; i++) {
		vector<Instruction> instr = generateRandomInstruction();
		for(auto& i : instr) {
			p->addInstruction(i);
		}
		if(instr.size() > 1) {
			i += instr.size()-1;
		}
	}

	//returns the newly made process
	return p;
}
