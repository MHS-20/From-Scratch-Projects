#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define ARRAY_SIZE 1024

int main(int argc, char **argv)
{
    (void)argc, (void)argv;
    char *buf = NULL, *token;
    int i, status;
    char **array;

    size_t count = 0;
    ssize_t nread;
    pid_t child_pid;

    while (1)
    {
        printf("MyShell$ ");
        nread = getline(&buf, &count, stdin);

        if (nread == -1)
        {
            perror("Exiting shell");
            exit(1);
        }

        // tokenize input
        token = strtok(buf, " \n");
        array = malloc(sizeof(char *) * ARRAY_SIZE);

        i = 0;
        while (token)
        {
            array[i] = token;
            token = strtok(NULL, " \n");
            i++;
        }

        array[i] = NULL;
        child_pid = fork();

        if (child_pid == -1)
        {
            perror("Failed to create.");
            exit(41);
        }

        // execute command
        if (child_pid == 0)
        {
            if (execve(array[0], array, NULL) == -1)
            {
                perror("Failed to execute");
                exit(97);
            }
        }
        else
        {
            wait(&status);
        }
    }
    free(buf);
    return (0);
}