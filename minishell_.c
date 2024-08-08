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
int next_bg_id = 1;
int bg_done = 0;

void prompt(void)
{
    printf("\nmsh> ");
    fflush(stdout);
}

void handle_sigchld(int sig)
{
    bg_done = 1;
}

void check_background_processes()
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for (int i = 0; i < bg_count; i++)
        {
            if (bg_processes[i].pid == pid)
            {
                if (WIFEXITED(status))
                {
                    printf("\n[%d]+ Done %s\n", bg_processes[i].id, bg_processes[i].command);
                }
                else if (WIFSIGNALED(status))
                {
                    printf("\n[%d]+ Terminated by signal %d %s\n", bg_processes[i].id, WTERMSIG(status), bg_processes[i].command);
                }

                // Remove the finished process from the array
                for (int j = i; j < bg_count - 1; j++)
                {
                    bg_processes[j] = bg_processes[j + 1];
                }
                bg_count--;
                break;
            }
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
            bg_processes[bg_count].id = next_bg_id++;
            bg_processes[bg_count].pid = pid;
            strcpy(bg_processes[bg_count].command, v[0]);
            for (int i = 1; v[i] != NULL; i++)
            {
                strcat(bg_processes[bg_count].command, " ");
                strcat(bg_processes[bg_count].command, v[i]);
            }
            printf("[%d] %d\n", bg_processes[bg_count].id, pid);
            fflush(stdout);
            bg_count++;
        }
        else
        {
            int status;
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("waitpid");
            }
        }
    }
}

void change_directory(char *path)
{
    if (path == NULL)
    {
        fprintf(stderr, "cd: expected argument\n");
    }
    else if (chdir(path) < 0)
    {
        perror("chdir");
    }
}

int main(int argk, char *argv[], char *envp[])
{
    char *v[NV];
    char *sep = " \t\n";
    int i;

    // Set up signal handler for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* prompt for and process one command line at a time  */
    while (1)
    {
        if (bg_done)
        {
            check_background_processes();
            bg_done = 0;
        }

        prompt();
        if (!fgets(line, NL, stdin))
        {
            if (feof(stdin))
            {
                fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
                exit(0);
            }
            perror("fgets");
            continue;
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

        if (strncmp(v[0], "cd", 2) == 0)
        {
            change_directory(v[1]);
        }
        else
        {
            int background = (v[i - 1] && strcmp(v[i - 1], "&") == 0);
            if (background)
            {
                v[i - 1] = NULL; // Remove '&' character
            }
            execute_command(v, background);
        }
    } /* while */
} /* main */