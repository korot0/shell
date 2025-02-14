#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#define WHITESPACE " \t\n"
#define MAX_COMMAND_SIZE 255
#define MAX_NUM_ARGUMENTS 12
#define PID_HISTORY_SIZE 15
#define COMMAND_HISTORY_SIZE 15

typedef struct
{
    pid_t pids[PID_HISTORY_SIZE];
    int count;
} PidHistory;

typedef struct
{
    char commands[COMMAND_HISTORY_SIZE][MAX_COMMAND_SIZE];
    int count;
} CommandHistory;

void printPidHistory(PidHistory *ph);
void addPid(PidHistory *ph, pid_t pid);
void printCommandHistory(CommandHistory *ch);
void addCommand(CommandHistory *ch, const char *command);
void executeCommandFromHistory(CommandHistory *ch, int index);

int main()
{
    PidHistory pidHistory = {.count = 0};
    CommandHistory commandHistory = {.count = 0};

    char *command_string = (char *)malloc(MAX_COMMAND_SIZE);

    while (1)
    {
        printf("msh> ");

        while (!fgets(command_string, MAX_COMMAND_SIZE, stdin))
            ;

        char *token[MAX_NUM_ARGUMENTS];
        int token_count = 0;
        char *argument_ptr;
        char *working_string = strdup(command_string);
        char *head_ptr = working_string;

        while (((argument_ptr = strsep(&working_string, WHITESPACE)) != NULL) &&
               (token_count < MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
            if (strlen(token[token_count]) == 0)
            {
                token[token_count] = NULL;
            }
            token_count++;
        }

        if (token[0] == NULL || strlen(token[0]) == 0)
        {
            free(head_ptr);
            continue;
        }

        if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
        {
            exit(0);
        }

        if (strcmp(token[0], "cd") == 0)
        {
            if (token[1] == NULL)
            {
                chdir(getenv("HOME"));
            }
            else
            {
                chdir(token[1]);
            }
            addCommand(&commandHistory, command_string);
            continue;
        }

        if (strcmp(token[0], "showpids") == 0)
        {
            addCommand(&commandHistory, command_string);
            printPidHistory(&pidHistory);
            continue;
        }

        if (strcmp(token[0], "history") == 0)
        {
            addCommand(&commandHistory, command_string);
            printCommandHistory(&commandHistory);
            continue;
        }

        // Handle !n command
        if (token[0][0] == '!')
        {
            int index = atoi(token[0] + 1);
            if (index >= 0 && index < commandHistory.count)
            {
                executeCommandFromHistory(&commandHistory, index);
            }
            else
            {
                printf("Command not in history.\n");
            }
            continue;
        }

        pid_t pid = fork();

        if (pid == -1)
        {
            return -1;
        }

        if (pid == 0)
        {
            if (execvp(token[0], token) == -1)
            {
                fprintf(stderr, "%s: Command not found.\n", token[0]);
                exit(1);
            }
        }
        else
        {
            if (wait(NULL) == -1)
            {
                continue;
            }
            addPid(&pidHistory, pid);
            addCommand(&commandHistory, command_string);
        }

        free(head_ptr);
    }

    return 0;
}

void addPid(PidHistory *ph, pid_t pid)
{
    if (ph->count == PID_HISTORY_SIZE)
    {
        for (int i = 1; i < PID_HISTORY_SIZE; i++)
        {
            ph->pids[i - 1] = ph->pids[i];
        }
        ph->count--;
    }
    ph->pids[ph->count] = pid;
    ph->count++;
}

void printPidHistory(PidHistory *ph)
{
    if (ph->count == 0)
    {
        printf("No pid history yet.\n");
        return;
    }

    for (int i = 0; i < ph->count; i++)
    {
        printf("%d: %d\n", i, ph->pids[i]);
    }
}

void addCommand(CommandHistory *ch, const char *command)
{
    if (command == NULL || strcmp(command, "\n") == 0)
    {
        return;
    }

    if (ch->count == COMMAND_HISTORY_SIZE)
    {
        for (int i = 1; i < COMMAND_HISTORY_SIZE; i++)
        {
            strcpy(ch->commands[i - 1], ch->commands[i]);
        }
        ch->count--;
    }
    strncpy(ch->commands[ch->count], command, MAX_COMMAND_SIZE - 1);
    ch->commands[ch->count][MAX_COMMAND_SIZE - 1] = '\0';
    ch->count++;
}

void printCommandHistory(CommandHistory *ch)
{
    for (int i = 0; i < ch->count; i++)
    {
        printf("%d: %s", i, ch->commands[i]);
    }
}

void executeCommandFromHistory(CommandHistory *ch, int index)
{
    if (index < 0 || index >= ch->count)
    {
        printf("Command not in history.\n");
        return;
    }

    char *command = ch->commands[index];
    printf("Executing: %s", command);

    // Tokenize the command and execute it
    char *token[MAX_NUM_ARGUMENTS];
    int token_count = 0;
    char *argument_ptr;
    char *working_string = strdup(command);
    char *head_ptr = working_string;

    while (((argument_ptr = strsep(&working_string, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
        token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
        if (strlen(token[token_count]) == 0)
        {
            token[token_count] = NULL;
        }
        token_count++;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        return;
    }

    if (pid == 0)
    {
        if (execvp(token[0], token) == -1)
        {
            fprintf(stderr, "%s: Command not found.\n", token[0]);
            exit(1);
        }
    }
    else
    {
        if (wait(NULL) == -1)
        {
            return;
        }
    }

    free(head_ptr);
}