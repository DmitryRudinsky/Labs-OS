#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sstream>
#include <cstring>

const int SHM_SIZE = 4096;
const char* SHM_NAME = "/my_shm";
const char* DATA_SEM_NAME = "/my_data_sem";
const char* PROCESSING_SEM_NAME = "/my_processing_sem";

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

    sem_t* data_sem = sem_open(DATA_SEM_NAME, 0);
    if (data_sem == SEM_FAILED) {
        perror("sem_open data_sem error");
        exit(EXIT_FAILURE);
    }

    sem_t* processing_sem = sem_open(PROCESSING_SEM_NAME, 0);
    if (processing_sem == SEM_FAILED) {
        perror("sem_open processing_sem error");
        exit(EXIT_FAILURE);
    }

    while (true) {
        sem_wait(data_sem);

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

        sem_post(processing_sem);
    }

    munmap(shm_ptr, SHM_SIZE);
    sem_close(data_sem);
    sem_close(processing_sem);

    return 0;
}