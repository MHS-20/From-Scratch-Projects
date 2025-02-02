#include "shell.h"
// Built-in command implementations

// char *builtin_func_list[] = {
//     "cd",
//     "exit",
//     "help",
//     "env"};

int change_directory(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "cd: missing argument\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("cd");
        }
    }
    return 1;
}

int shell_exit(char **args)
{
    return 0;
}

int shell_help(char **args)
{
    printf("Built-in commands:\n");
    for (int i = 0; i < num_builtins; i++)
    {
        printf("  %s\n", builtin_func_list[i]);
    }
    return 1;
}

int shell_env(char **args)
{
    extern char **environ;
    for (char **env = environ; *env != 0; env++)
    {
        printf("%s\n", *env);
    }
    return 1;
}