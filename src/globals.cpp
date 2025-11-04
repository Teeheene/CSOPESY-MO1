#include <atomic>

std::atomic<int> nextId{0};      // global PID counter
int minIns = 1000;               // global min instructions
int maxIns = 2000;               // global max instructions

