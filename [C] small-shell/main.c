#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define ARRAY_SIZE 1024

char *get_file_path(char *cmd);
char *get_file_loc(char *path, char *file_name);
int isAbsolute(const char *str);

int main(int argc, char **argv)
{
    (void)argc, (void)argv;
    char *buf = NULL, *token;
    int i, status;
    char **array;
    char *path;

    size_t count = 0;
    ssize_t nread;
    pid_t child_pid;

    while (1)
    {
        if (isatty(STDIN_FILENO))
            write(STDOUT_FILENO, "MyShell$ ", 9);

        nread = getline(&buf, &count, stdin);

        if (nread == -1)
        {
            exit(0);
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

        // get command path
        path = get_file_path(array[0]);

        // execute command
        if (child_pid == 0)
        {
            if (execve(path, array, NULL) == -1)
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

// retrive command path from PATH var
char *get_file_path(char *cmd)
{

    char *full_path;
    char *path = getenv("PATH");
    if (!path)
    {
        perror("Command not found");
        return (NULL);
    }

    if (isAbsolute(cmd) && access(cmd, X_OK) == 0)
        return (strdup(cmd));

    full_path = get_file_loc(path, cmd);
    if (full_path == NULL)
    {
        perror("Command not found");
        return (NULL);
    }

    return (full_path);
}

// build abosolute path
char *get_file_loc(char *path, char *cmd)
{
    char *path_copy, *token;
    struct stat file_path;
    char *path_buffer = NULL;

    path_copy = strdup(path);
    token = strtok(path_copy, ":");

    while (token)
    {
        if (path_buffer)
        {
            free(path_buffer);
            path_buffer = NULL;
        }

        path_buffer = malloc(strlen(token) + strlen(cmd) + 2);

        if (!path_buffer)
        {
            perror("Error: malloc failed");
            exit(EXIT_FAILURE);
        }

        // build absolute path
        strcpy(path_buffer, token);
        strcat(path_buffer, "/");
        strcat(path_buffer, cmd);
        strcat(path_buffer, "\0");

        // check cmd file and access it
        if (stat(path_buffer, &file_path) == 0 && access(path_buffer, X_OK) == 0)
        {
            free(path_copy);
            return (path_buffer);
        }

        token = strtok(NULL, ":");
    }

    free(path_copy);
    if (path_buffer)
        free(path_buffer);

    return (NULL);
}

// check is the input cmd is already an absolute path
int isAbsolute(const char *str)
{
    if (str != NULL || str[0] == '/')
        return (1);

    return (0);
}