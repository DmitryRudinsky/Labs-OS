#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

void create_pipe(int* pipe_fd) {
    if (pipe(pipe_fd) == -1) {
        perror("Pipe error!\n");
        exit(EXIT_FAILURE);
    }
}

int main() {
    char filename[256];

    write(STDOUT_FILENO, "Введите имя файла: ", 35);
    ssize_t bytesRead = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytesRead > 0) {
        filename[bytesRead - 1] = '\0';
    } else {
        perror("Ошибка при вводе имени файла");
        exit(EXIT_FAILURE);
    }

    int pipe1_fd[2];
    create_pipe(pipe1_fd);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork error!\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(pipe1_fd[0]);

        int file_fd = open(filename, O_RDONLY);
        if (file_fd == -1) {
            perror("File open error!\n");
            exit(EXIT_FAILURE);
        }

        dup2(file_fd, STDIN_FILENO);
        dup2(pipe1_fd[1], STDOUT_FILENO);

        close(file_fd);
        close(pipe1_fd[1]);

        execl("./child", "./child", nullptr);

        perror("execl error!\n");
        exit(EXIT_FAILURE);
    } else {
        close(pipe1_fd[1]);

        char buffer[256];
        ssize_t bytesRead;
        while ((bytesRead = read(pipe1_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            write(STDOUT_FILENO, buffer, bytesRead);
        }

        close(pipe1_fd[0]);

        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}
