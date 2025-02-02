#ifndef SHELL_H
#define SHELL_H

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
char **tokenize(char *buf);

/*---Builtin func---*/
int change_directory(char **args);
int shell_exit(char **args);
int shell_help(char **args);
int shell_env(char **args);

extern char *builtin_func_list[];
extern int num_builtins;

#endif