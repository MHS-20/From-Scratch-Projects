#include "shell.h"

// set up for built-in commands
char *builtin_func_list[] = {
    "cd",
    "exit",
    "help",
    "env"};

int (*builtin_func[])(char **) = {
    &change_directory,
    &shell_exit,
    &shell_help,
    &shell_env};

int num_builtins = sizeof(builtin_func_list) / sizeof(char *);

// Function to check for built-in commands and execute them
int execute_builtin(char **args)
{
    for (int i = 0; i < num_builtins; i++)
    {
        if (strcmp(args[0], builtin_func_list[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }
    return -1; // Not a built-in command
}

int main(int argc, char **argv)
{
    (void)argc, (void)argv;
    char *buf = NULL;
    char **tokens;
    int status;
    char *path;

    size_t count = 0;
    ssize_t nread;
    pid_t child_pid;

    // shell loop
    while (1)
    {
        if (isatty(STDIN_FILENO))
            write(STDOUT_FILENO, "MyShell$ ", 9);

        nread = getline(&buf, &count, stdin);
        if (nread == -1)
        {
            exit(EXIT_FAILURE);
        }

        // tokenize input
        tokens = tokenize(buf);

        // Check for built-in commands
        int builtin_status = execute_builtin(tokens);
        if (builtin_status == 0)
        {
            break; // Exit shell
        }
        else if (builtin_status == 1)
        {
            continue; // Built-in command executed
        }

        child_pid = fork();
        if (child_pid == -1)
        {
            perror("Failed to create.");
            exit(41);
        }

        // get command path
        path = get_file_path(tokens[0]);

        // execute command
        if (child_pid == 0)
        {
            if (execve(path, tokens, NULL) == -1)
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

// tokenize input
char **tokenize(char *buf)
{

    char **tokens;
    int arraysize = 64;
    char *token;
    int i;

    token = strtok(buf, " \n");
    tokens = malloc(sizeof(char *) * arraysize);

    i = 0;
    while (token)
    {
        // comments
        if (token[0] == '#')
            break;

        tokens[i] = token;
        token = strtok(NULL, " \n");
        i++;

        // enlarge array if needed
        if (i >= arraysize)
        {
            arraysize += arraysize;
            tokens = realloc(tokens, arraysize * sizeof(char *));

            if (!tokens)
            {
                fprintf(stderr, "reallocation error for tokens");
                exit(EXIT_FAILURE);
            }
        }
    }

    tokens[i] = NULL;
    return tokens;
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