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
int bg_done = 0;

void handle_sigchld(int sig)
{
    bg_done = 1;
}

// void check_background_processes()
// {
//     int status;
//     pid_t pid;
//     while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
//     {
//         for (int i = 0; i < bg_count; ++i)
//         {
//             if (bg_processes[i].pid == pid)
//             {
//                 if (WIFEXITED(status))
//                 {
//                     printf("[%d]+ Done %s\n", bg_processes[i].id, bg_processes[i].command);
//                 }
//                 else if (WIFSIGNALED(status))
//                 {
//                     printf("[%d]+ Terminated by signal %d %s\n", bg_processes[i].id, WTERMSIG(status), bg_processes[i].command);
//                 }
//                 for (int j = i; j < bg_count - 1; ++j)
//                 {
//                     bg_processes[j] = bg_processes[j + 1];
//                 }
//                 --bg_count;
//                 break;
//             }
//         }
//     }
// }
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
                    printf("[%d]+            Done %s\n", bg_processes[i].id, bg_processes[i].command);
                }
                else if (WIFSIGNALED(status))
                {
                    printf("[%d]+            Terminated by signal %d %s\n", bg_processes[i].id, WTERMSIG(status), bg_processes[i].command);
                }

                // 将已完成的进程从数组中移除，并保持数组顺序
                for (int j = i; j < bg_count - 1; ++j)
                {
                    bg_processes[j] = bg_processes[j + 1];
                }
                --bg_count;
                break; // 处理完一个子进程后跳出循环，防止多次递减bg_count
            }
        }
    }
}

int main(void)
{
    char *args[NV + 1];
    int should_run = 1;

    signal(SIGCHLD, handle_sigchld);

    while (should_run)
    {
        if (bg_done)
        {
            check_background_processes();
            bg_done = 0;
        }

        // 禁用提示符行
        // fprintf(stdout, "\n msh> ");
        fflush(stdout);

        if (!fgets(line, NL, stdin))
        {
            // if (feof(stdin))
            // {
            //     printf("\n");
            //     exit(0);
            // }
            // 禁用空 stdin 时打印的错误消息
            /*
            fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),
                    feof(stdin), ferror(stdin));
            */
            continue;
        }

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

        char full_command[NL];
        strncpy(full_command, line, NL);
        full_command[NL - 1] = '\0';

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
            // if (args[1] == NULL)
            // {
            //     fprintf(stderr, "cd: expected argument\n");
            // }
            if (chdir(args[1]) != 0)
            {
                perror("chdir");
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
            // 子进程
            if (execvp(args[0], args) == -1)
            {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // 父进程
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
                // 禁用命令完成后打印的行
                // printf("%s done \n", v[0]);
            }
        }
    }
    return 0;
}