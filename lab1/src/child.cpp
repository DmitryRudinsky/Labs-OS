#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        float number, sum = 0;

        while (iss >> number) {
            sum += number;
        }

        std::cout << "Сумма: " << sum << std::endl;
    }

    return 0;
}
