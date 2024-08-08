/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File         : minishell.c
   Compiler/System : gcc/linux

********************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */
char line[NL]; /* command input buffer */

typedef struct
{
    int id;
    pid_t pid;
    char command[256];
} bg_process;

bg_process bg_processes[100];
int bg_count = 0;

void prompt(void)
{
    printf("\n msh> ");
    fflush(stdout);
}

void handle_bg_processes()
{
    int status;
    for (int i = 0; i < bg_count; i++)
    {
        pid_t result = waitpid(bg_processes[i].pid, &status, WNOHANG);
        if (result > 0)
        {
            printf("[%d]+ Done %s\n", bg_processes[i].id, bg_processes[i].command);
            for (int j = i; j < bg_count - 1; j++)
            {
                bg_processes[j] = bg_processes[j + 1];
            }
            bg_count--;
            i--;
        }
    }
}

void execute_command(char *v[], int background)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return;
    }
    if (pid == 0)
    {
        // Child process
        if (execvp(v[0], v) < 0)
        {
            perror("execvp");
            _exit(EXIT_FAILURE);
        }
    }
    else
    {
        // Parent process
        if (background)
        {
            bg_processes[bg_count].id = bg_count + 1;
            bg_processes[bg_count].pid = pid;
            strcpy(bg_processes[bg_count].command, v[0]);
            printf("[%d] %d\n", bg_processes[bg_count].id, pid);
            bg_count++;
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void change_directory(char *path)
{
    if (chdir(path) < 0)
    {
        perror("chdir");
    }
}

int main(int argk, char *argv[], char *envp[])
{
    char *v[NV];
    char *sep = " \t\n";
    int i;

    while (1)
    {
        prompt();
        fgets(line, NL, stdin);
        fflush(stdin);

        if (feof(stdin))
        {

            fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),
                    feof(stdin), ferror(stdin));
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue;

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++)
        {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        handle_bg_processes();

        if (strncmp(v[0], "cd", 2) == 0)
        {
            change_directory(v[1]);
        }
        else
        {
            int background = (v[i - 1] && strcmp(v[i - 1], "&") == 0);
            if (background)
            {
                v[i - 1] = NULL;
            }
            execute_command(v, background);
        }
    } /* while */
} /* main */
