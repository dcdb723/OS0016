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

typedef struct BackgroundProcess
{
    int id;
    pid_t pid;
    char command[NL];
} BackgroundProcess;

BackgroundProcess bg_processes[NV];
int bg_count = 0;
int next_bg_id = 1;

void handle_sigchld(int sig)
{
    // This handler is left empty to avoid automatic message printing
    // Background process status will be checked in the main loop
}

void check_background_processes()
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for (int i = 0; i < bg_count; ++i)
        {
            if (bg_processes[i].pid == pid)
            {
                if (WIFEXITED(status))
                {
                    printf("[%d]+ Done                        %s\n", bg_processes[i].id, bg_processes[i].command);
                }
                else if (WIFSIGNALED(status))
                {
                    printf("[%d]+                        Terminated by signal %d %s\n", bg_processes[i].id, WTERMSIG(status), bg_processes[i].command);
                }
                // Remove the finished process from the array
                for (int j = i; j < bg_count - 1; ++j)
                {
                    bg_processes[j] = bg_processes[j + 1];
                }
                --bg_count;
                break;
            }
        }
    }
}

int main(void)
{
    char *args[NV + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */

    signal(SIGCHLD, handle_sigchld);

    while (should_run)
    {
        // Check and print background processes before showing the prompt
        check_background_processes();

        printf("osh>");
        fflush(stdout);

        if (!fgets(line, NL, stdin))
        {
            perror("fgets");
            continue;
        }

        // Remove newline character from input
        size_t length = strlen(line);
        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        int background = 0;
        if (length > 1 && line[length - 2] == '&')
        {
            background = 1;
            line[length - 2] = '\0';
        }

        // Store the full command line for the background process
        char full_command[NL];
        strncpy(full_command, line, NL);
        full_command[NL - 1] = '\0'; // Ensure null-terminated string

        int i = 0;
        char *token = strtok(line, " ");
        while (token != NULL)
        {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] == NULL)
        {
            continue;
        }

        if (strcmp(args[0], "exit") == 0)
        {
            should_run = 0;
            continue;
        }

        if (strcmp(args[0], "cd") == 0)
        {
            if (args[1] == NULL)
            {
                fprintf(stderr, "cd: expected argument\n");
            }
            else
            {
                if (chdir(args[1]) != 0)
                {
                    perror("chdir");
                }
            }
            continue;
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            continue;
        }
        else if (pid == 0)
        {
            // Child process
            if (execvp(args[0], args) == -1)
            {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // Parent process
            if (!background)
            {
                if (waitpid(pid, NULL, 0) == -1)
                {
                    perror("waitpid");
                }
            }
            else
            {
                bg_processes[bg_count].id = next_bg_id++;
                bg_processes[bg_count].pid = pid;
                strncpy(bg_processes[bg_count].command, full_command, NL);
                bg_processes[bg_count].command[NL - 1] = '\0';
                ++bg_count;
                printf("[%d] %d\n", bg_processes[bg_count - 1].id, pid);
            }
        }
    }
    return 0;
}