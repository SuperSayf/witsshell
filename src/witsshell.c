#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGUMENTS 100
#define MAX_PATH_LENGTH 1024
#define MAX_PATHS 100

char searchPaths[MAX_PATHS][MAX_PATH_LENGTH] = {"/bin"}; // Initial search paths

// Function to display error message
void displayError(const char *message)
{
    write(STDERR_FILENO, message, strlen(message));
}

// Function to execute the CD command
void executeCD(char *args[])
{
    if (args[1] == NULL)
    {
        displayError("An error has occurred\n");
    }
    else if (args[2] != NULL)
    {
        displayError("An error has occurred\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            displayError("An error has occurred\n");
        }
    }
}

// Function to set the search path
void setPath(char *args[])
{
    int pathCount = 0;

    for (int i = 1; args[i] != NULL; i++)
    {
        char *path = strtok(args[i], " ");
        while (path != NULL)
        {
            if (pathCount == MAX_PATHS)
            {
                displayError("An error has occurred\n");
                return;
            }

            strcpy(searchPaths[pathCount], path);
            pathCount++;

            path = strtok(NULL, " ");
        }
    }

    // Null-terminate the search path
    searchPaths[pathCount][0] = '\0';
}

// Function to check for built-in commands
bool isBuiltInCommand(const char *command)
{
    return strcmp(command, "exit") == 0 || strcmp(command, "cd") == 0 ||
           strcmp(command, "path") == 0 || strcmp(command, "clear") == 0;
}

// Function to redirect output to a file
void redirectOutput(const char *outputFile)
{
    int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd == -1)
    {
        displayError("An error has occurred\n");
        exit(1);
    }

    // Redirect only the standard output (STDOUT) to the specified file
    dup2(fd, STDOUT_FILENO);

    // Close the file descriptor, as we no longer need it after redirection
    close(fd);
}

// Function to ensure ">" operator is surrounded by spaces
void ensureSpacesAroundGreaterThan(char *input)
{
    // Iterate through the input string
    for (int i = 0; input[i] != '\0'; i++)
    {
        if (input[i] == '>')
        {
            // Check if the previous character is not a space
            if (i > 0 && input[i - 1] != ' ')
            {
                // Insert a space before the operator
                memmove(input + i + 1, input + i, strlen(input + i) + 1);
                input[i] = ' ';
            }

            // Check if the next character is not a space
            if (input[i + 1] != ' ')
            {
                // Insert a space after the operator
                memmove(input + i + 2, input + i + 1, strlen(input + i + 1) + 1);
                input[i + 1] = ' ';
            }
        }
    }
}

// Function to execute a command
void executeCommand(char *args[], const char *outputFile)
{
    int originalStdout = dup(STDOUT_FILENO); // Save the original STDOUT

    if (outputFile)
    {
        // Redirect output to the specified file
        redirectOutput(outputFile);
    }

    pid_t childPid = fork();
    if (childPid == -1)
    {
        displayError("An error has occurred\n");
        exit(1);
    }
    else if (childPid == 0)
    {
        // Child process
        char fullPath[MAX_PATH_LENGTH];
        bool found = false;

        // Iterate through the search paths
        for (int i = 0; searchPaths[i][0] != '\0'; i++)
        {
            strcpy(fullPath, searchPaths[i]);
            strcat(fullPath, "/");
            strcat(fullPath, args[0]);

            // Check if the command exists in the current search path
            if (access(fullPath, X_OK) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            displayError("An error has occurred\n");
            exit(1);
        }

        // Execute the command
        execv(fullPath, args);
        displayError("An error has occurred\n");
        exit(1);
    }
    else
    {
        // Parent process for sequential execution
        int status;
        waitpid(childPid, &status, 0);
    }

    // Restore the original STDOUT and close the duplicated file descriptor
    dup2(originalStdout, STDOUT_FILENO);
    close(originalStdout);
}

// Function to parse command arguments and handle output redirection
void parseCommandArgs(char *args[], int argCount, char *commandArgs[], int *commandArgCount, const char **outputFile, bool *shouldRun)
{
    bool outputDetected = false; // Track if an output file has been detected

    for (int i = 0; i < argCount; i++)
    {
        // Check for output redirection
        if (strcmp(args[i], ">") == 0)
        {
            if (i == 0 || i == argCount - 1)
            {
                displayError("An error has occurred\n");
                *shouldRun = false;
                break;
            }
            else
            {
                if (outputDetected)
                {
                    displayError("An error has occurred\n");
                    *shouldRun = false;
                    break;
                }
                *outputFile = args[i + 1]; // Store the output file
                outputDetected = true;
                i++; // Skip the next argument (the output file)
                continue;
            }
        }

        // Check if the argument contains ">" without being an output file
        if (strstr(args[i], ">") != NULL)
        {
            displayError("An error has occurred\n");
            *shouldRun = false;
            break;
        }

        if (outputDetected)
        {
            // If an output file has been detected, this argument is an additional output file
            displayError("An error has occurred\n");
            *shouldRun = false;
            break;
        }

        commandArgs[*commandArgCount] = args[i];
        (*commandArgCount)++;
    }

    commandArgs[*commandArgCount] = NULL; // Null-terminate the command argument list
}

void executeCommandsInParallel(char *parallelCommands[])
{
    // Declare a variable to store the status of child processes
    int status;

    // Iterate through parallel commands
    for (int i = 0; parallelCommands[i] != NULL; i++)
    {

        // Print the parallel command
        // printf("%s\n", parallelCommands[i]);

        // Create a child process
        pid_t childPid = fork();
        if (childPid == -1)
        {
            displayError("An error has occurred\n");
            exit(1);
        }
        else if (childPid == 0)
        {
            // Child process
            char *args[MAX_ARGUMENTS];
            int argCount = 0;
            args[argCount] = strtok(parallelCommands[i], " ");
            while (args[argCount] != NULL)
            {
                argCount++;
                args[argCount] = strtok(NULL, " ");
            }
            args[argCount] = NULL; // Null-terminate the argument list

            // Variables to pass to parseCommandArgs
            char *commandArgs[MAX_ARGUMENTS];
            int commandArgCount = 0;
            const char *outputFile = NULL;
            bool shouldRun = true;

            parseCommandArgs(args, argCount, commandArgs, &commandArgCount, &outputFile, &shouldRun);

            if (shouldRun)
            {
                // Execute the command with the parsed arguments and output file
                executeCommand(commandArgs, outputFile);
            }

            exit(0);
        }
    }

    // Wait for all child processes to complete and collect their status
    for (int i = 0; parallelCommands[i] != NULL; i++)
    {
        wait(&status); // Collect the exit status of each child process
    }
}

int main(int argc, char *argv[])
{
    bool batchMode = false;
    FILE *inputStream = stdin;

    // Check for batch mode
    if (argc == 2)
    {
        batchMode = true;
        inputStream = fopen(argv[1], "r");
        if (!inputStream)
        {
            displayError("An error has occurred\n");
            exit(1);
        }
    }
    else if (argc > 2)
    {
        displayError("An error has occurred\n");
        exit(1);
    }

    char input[MAX_INPUT_LENGTH];
    char *parallelCommands[MAX_ARGUMENTS];
    char *args[MAX_ARGUMENTS];

    while (1)
    {
        char currentDirectory[MAX_PATH_LENGTH];
        getcwd(currentDirectory, sizeof(currentDirectory));
        if (!batchMode)
        {
            printf("witsshell (%s)> ", currentDirectory);
            fflush(stdout);
        }

        // Read input
        if (fgets(input, sizeof(input), inputStream) == NULL)
        {
            break; // End of file or error
        }

        input[strlen(input) - 1] = '\0'; // Remove trailing newline

        // Ensure ">" operator is surrounded by spaces
        ensureSpacesAroundGreaterThan(input);

        // Check if the input contains a "&" operator
        bool parallel = false;
        int parallelCommandCount = 0;
        if (strstr(input, "&") != NULL)
        {
            // Duplicate the input string before splitting into parallel commands
            char inputCopy[MAX_INPUT_LENGTH];
            strcpy(inputCopy, input);

            // Split the input copy into parallel commands
            parallelCommands[0] = strtok(inputCopy, "&");
            while (parallelCommands[parallelCommandCount] != NULL)
            {
                parallelCommandCount++;
                parallelCommands[parallelCommandCount] = strtok(NULL, "&");
            }

            parallel = true;
        }

        // Remove spaces from the beginning of each parallel command
        for (int i = 0; i < parallelCommandCount; i++)
        {
            while (parallelCommands[i][0] == ' ')
            {
                memmove(parallelCommands[i], parallelCommands[i] + 1, strlen(parallelCommands[i]));
            }
        }

        int argCount = 0;
        args[argCount] = strtok(input, " ");
        while (args[argCount] != NULL)
        {
            argCount++;
            args[argCount] = strtok(NULL, " ");
        }
        args[argCount] = NULL; // Null-terminate the argument list

        if (argCount > 0)
        {
            if (isBuiltInCommand(args[0]))
            {
                if (strcmp(args[0], "exit") == 0)
                {
                    if (argCount > 1)
                    {
                        displayError("An error has occurred\n");
                    }
                    else
                    {
                        exit(0); // Exit the shell
                    }
                }
                else if (strcmp(args[0], "cd") == 0)
                {
                    executeCD(args);
                }
                else if (strcmp(args[0], "path") == 0)
                {
                    setPath(args);
                }
                else if (strcmp(args[0], "clear") == 0)
                {
                    system("clear");
                }
                continue;
            }

            if (!parallel)
            {
                // First parse the command arguments
                char *commandArgs[MAX_ARGUMENTS];
                int commandArgCount = 0;
                const char *outputFile = NULL;
                bool shouldRun = true;

                parseCommandArgs(args, argCount, commandArgs, &commandArgCount, &outputFile, &shouldRun);

                if (shouldRun)
                {
                    executeCommand(commandArgs, outputFile);
                }
            }
            else if (parallel)
            {
                // Print the parallel command
                // for (int i = 0; i < parallelCommandCount; i++)
                // {
                //     printf("%s\n", parallelCommands[i]);
                // }
                executeCommandsInParallel(parallelCommands);
            }
        }
    }

    if (batchMode)
    {
        fclose(inputStream);
    }

    return 0;
}