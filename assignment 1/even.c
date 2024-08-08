#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void handle_sighup(int sig)
{
    printf("Ouch!\n");
}

void handle_sigint(int sig)
{
    printf("Yeah!\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0)
    {
        fprintf(stderr, "Please enter a positive integer.\n");
        return 1;
    }

    // Register signal handlers
    signal(SIGHUP, handle_sighup);
    signal(SIGINT, handle_sigint);

    for (int i = 0; i < n; ++i)
    {
        printf("%d\n", 2 * i);
        fflush(stdout);
        sleep(5);
    }

    return 0;
}