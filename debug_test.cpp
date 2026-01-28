#include <iostream>
#include <fstream>
int main() {
    std::ofstream log("test_log.txt");
    log << "TEST WORKED" << std::endl;
    return 0;
}
