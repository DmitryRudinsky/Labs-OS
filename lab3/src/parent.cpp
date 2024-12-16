#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

const int SHM_SIZE = 4096;
const char* SHM_NAME = "/my_shm";
const char* SEM_NAME = "/my_sem";

int main() {
    char filename[256];

    std::cout << "Введите имя файла: ";
    std::cin.getline(filename, sizeof(filename));

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open error");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate error");
        exit(EXIT_FAILURE);
    }

    char* shm_ptr = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open error");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execl("./child", "./child", nullptr);
        perror("execl error");
        exit(EXIT_FAILURE);
    } else {
        int file_fd = open(filename, O_RDONLY);
        if (file_fd == -1) {
            perror("open error");
            exit(EXIT_FAILURE);
        }

        ssize_t bytesRead;
        while ((bytesRead = read(file_fd, shm_ptr, SHM_SIZE - 1)) > 0) {
            shm_ptr[bytesRead] = '\0';
            sem_post(sem);
            sem_wait(sem);
        }

        close(file_fd);
        sem_post(sem);

        waitpid(pid, nullptr, 0);

        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        sem_close(sem);
        sem_unlink(SEM_NAME);
    }

    return 0;
}