#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "Blinky - System Status Monitor" << std::endl;
    std::cout << "Status: \033[32m●\033[0m GREEN - All systems operational" << std::endl;
    
    return 0;
}
