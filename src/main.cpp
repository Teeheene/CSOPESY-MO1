#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <optional>
#include <unordered_map>
#include <random>
using namespace std;

/* GLOBAL VARIABLES ********/
atomic<bool> running(true);
/***************************/

/* HEADERS *****************/
#include "helper.hpp"
#include "initialize.hpp"
#include "process.hpp"
#include "scheduler.hpp"
#include "mainController.hpp"
/****************************/

int main()
{
	MainController os;
	os.run();
}
