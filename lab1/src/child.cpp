#include <iostream>
#include <unistd.h>
#include <sstream>
#include <string>
#include <cstring>

int main() {
    char buffer[256];
    ssize_t bytesRead;

    while ((bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';

        char* line = strtok(buffer, "\n");
        while (line) {
            std::istringstream iss(line);
            float number, sum = 0;

            while (iss >> number) {
                sum += number;
            }

            char output[256];
            int output_len = snprintf(output, sizeof(output), "Сумма: %.2f\n", sum);
            write(STDOUT_FILENO, output, output_len);

            line = strtok(nullptr, "\n");
        }
    }

    return 0;
}
