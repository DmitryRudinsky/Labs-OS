#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sstream>
#include <cstring>

const int SHM_SIZE = 4096;
const char* SHM_NAME = "/my_shm";
const char* SEM_NAME = "/my_sem";

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open error");
        exit(EXIT_FAILURE);
    }

    char* shm_ptr = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    sem_t* sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open error");
        exit(EXIT_FAILURE);
    }

    while (true) {
        sem_wait(sem);

        if (shm_ptr[0] == '\0') {
            break;
        }

        char* line = strtok(shm_ptr, "\n");
        while (line) {
            std::istringstream iss(line);
            float number, sum = 0;

            while (iss >> number) {
                sum += number;
            }

            std::cout << "Сумма: " << sum << std::endl;

            line = strtok(nullptr, "\n");
        }

        sem_post(sem);
    }

    munmap(shm_ptr, SHM_SIZE);
    sem_close(sem);

    return 0;
}