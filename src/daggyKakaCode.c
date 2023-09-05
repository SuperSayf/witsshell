#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#define PROMPT "witsshell> "
#define SPACEDELIM " "

// Array to store paths
char *paths[100] = {"/bin/"};
const char error_message[] = "An error has occurred\n";
int numArgs = 0;
int numPaths = 1;

// Function to print the error message
void printError()
{
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
}

// function to update the paths array
void updatePaths(char *args[])
{
    // Clear the paths array
    for (int i = 0; i < numPaths; i++)
    {
        paths[i] = NULL;
    }

    numPaths = numArgs - 1;
    for (int i = 0; i < numPaths; i++)
    {
        if (args[i + 1][strlen(args[i + 1]) - 1] != '/')
        {
            char *temp = malloc(strlen(args[i + 1]) + 2); // +2 for the '/' and '\0'
            strcpy(temp, args[i + 1]);
            strcat(temp, "/");
            paths[i] = temp;
        }
        else
        {
            paths[i] = strdup(args[i + 1]);
        }
    }
}

// Function to compare two strings, returns 1 if equal, 0 if not
int compareStrings(const char *string1, const char *string2)
{
    return strcmp(string1, string2) == 0;
}

// Function to split the input string by the " " delimiter and store the arguments in an array
void splitInput(const char *input, char *args[])
{
    int input_length = strlen(input);
    char output[2 * input_length];

    int output_index = 0;

    for (int i = 0; i < input_length; i++)
    {
        if (input[i] == '>' || input[i] == '&')
        {
            if (i > 0 && input[i - 1] != ' ')
            {
                output[output_index++] = ' ';
            }
            output[output_index++] = input[i];
            if (i < input_length - 1 && input[i + 1] != ' ' && input[i + 1] != '\0')
            {
                output[output_index++] = ' ';
            }
        }
        else
        {
            output[output_index++] = input[i];
        }
    }

    char *token;
    int i = 0;
    char *inputCopy = strdup(output); // Duplicate the input since strsep modifies the input

    while ((token = strsep(&inputCopy, " ")) != NULL)
    {
        size_t len = strcspn(token, "\n"); // Remove trailing newline character if present
        if (len > 0)
        {
            token[len] = '\0'; // Null-terminate the token
            args[i] = token;
            i++;
        }
    }

    numArgs = i;
    args[i] = NULL;
}

char *joinStrings(char *str1, char *str2)
{
    char *result = malloc(strlen(str1) + strlen(str2) + 1); // +1 for the null-terminator
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

bool validRedirect(char *args[])
{
    for (int i = 0; i < numArgs; i++)
    {
        // If has redirect symbol with fileName directly after it and nothing more
        // printf("Before the compare: %s\n", args[i]);
        if (compareStrings(args[i], ">"))
        {
            // printf("true\n");
            if (i + 2 == numArgs && i != 0)
                return true;
            else
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(0);
            }
        }
        //("After the compare: %s\n", args[i]);
    }

    return false;
}

void executeCommand(char *args[])
{
    bool error = true;
    int result;

    int original_stdout = dup(STDOUT_FILENO); // Save the original stdout

    if (validRedirect(args))
    {
        int file_descriptor = open(args[numArgs - 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (file_descriptor == -1)
        {
            perror("open");
            exit(EXIT_FAILURE);
        }
        // Redirect stdout to the file
        dup2(file_descriptor, STDOUT_FILENO);
        close(file_descriptor);

        args[numArgs - 2] = NULL;
        args[numArgs - 1] = NULL;
    }

    for (int i = 0; i < numPaths; i++)
    {
        char *path = joinStrings(paths[i], args[0]);
        if (access(path, X_OK) != -1)
        {
            error = false;
            int pid = fork();
            if (pid == 0)
            {
                result = execv(path, args);
                // It's a good practice to add an exit here if execv fails.
                exit(result);
            }
            else
            {
                wait(&result);
            }
            error = false;
            break;
        }
    }

    if (error || numPaths == 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    // Restore stdout to its original state
    dup2(original_stdout, STDOUT_FILENO);
    close(original_stdout);
}

void executeBuiltIn(char *args[])
{
    if (compareStrings(args[0], "cd"))
    {
        if (numArgs == 2)
        {
            if (chdir(args[1]) == -1)
                write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }

    else if (compareStrings(args[0], "path"))
    {
        updatePaths(args);
    }

    else
    {
        executeCommand(args);
    }
}

void processParallel(char *args[])
{
    bool hasParallel = false;

    // Fix input
    for (int i = 0; i < numArgs; i++)
    {
        if (compareStrings(args[i], "&"))
        {
            if (i == 0)
            {
                // write(STDERR_FILENO, error_message, strlen(error_message));
                exit(0);
            }
            else if (i == numArgs - 1)
            {
                args[i] = NULL;
                numArgs--;
            }
            else
            {
                hasParallel = true;
            }
        }
    }

    if (hasParallel)
    {
        int start = 0;
        int end = 0;
        for (end; args[end] != NULL; end++)
        {
            if (compareStrings(args[end], "&"))
            {
                char *args2[end - start];
                numArgs = end - start;
                for (int j = 0; j < end - start; j++)
                {
                    args2[j] = args[start + j];
                }
                args2[end - start] = NULL;
                start = end + 1;

                if (compareStrings(args2[0], "cd") || compareStrings(args2[0], "path"))
                {
                    executeBuiltIn(args2);
                }
                else
                {
                    int pid = fork();
                    if (pid == 0)
                    {
                        executeBuiltIn(args2);
                        fclose(NULL);
                        exit(0);
                    }
                }
            }
        }

        // Execute the last iteration
        char *args2[end - start];
        numArgs = end - start;
        for (int j = 0; j < end - start; j++)
        {
            args2[j] = args[start + j];
        }
        args2[end - start] = NULL;
        if (compareStrings(args2[0], "cd") || compareStrings(args2[0], "path"))
        {
            executeBuiltIn(args2);
        }
        else
        {
            int pid = fork();
            if (pid == 0)
            {
                executeBuiltIn(args2);
                fclose(NULL);
                exit(0);
            }
        }
        // Parent waits for all children to finish
        while (wait(NULL) > 0)
            ;
    }

    else
    {
        executeBuiltIn(args);
    }
}

char *processString5000(const char *string)
{
    size_t len = strlen(string);
    char *str = (char *)malloc(len + 1); // +1 for null-terminator
    int spaceCounter = 0;
    int j = 0;
    // Remove leading spaces or tabs

    for (size_t i = 0; i < len; i++)
    {
        char currentChar = string[i];
        bool isSpaceOrTab = (currentChar == ' ' || currentChar == '\t');

        if (spaceCounter < 1)
        {
            if (isSpaceOrTab)
            {
                spaceCounter++;
                str[j++] = ' ';
            }
            else
            {
                str[j++] = currentChar;
            }
        }
        else
        {
            if (!isSpaceOrTab)
            {
                str[j++] = currentChar;
                spaceCounter = 0;
            }
            else
            {
                spaceCounter++;
            }
        }
    }

    str[j] = '\0'; // Null-terminate the new string
    return str;
}

// Function to count the number of arguments
int countArgs(const char *input)
{
    int numArgs = 0;
    char *inputCopy = strdup(input);
    char *token;
    while ((token = strsep(&inputCopy, " ")) != NULL)
    {
        if (compareStrings(token, "exit"))
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
        }
        numArgs++;
    }
    return numArgs;
}

int main(int argc, char *argv[])
{
    bool end = false;
    char *input = NULL;
    size_t len = 0;

    if (argc > 2)
    {
        printError();
    }

    if (argc == 1)
    {
        while (!end)
        {
            printf(PROMPT);
            if (getline(&input, &len, stdin) == -1)
            {
                break;
            }
            if (compareStrings(input, "exit\n") || feof(stdin))
            {
                end = true;
            }
            else if (strspn(input, " \t\n\r\f\v") != strlen(input))
            {
                char *processedString = processString5000(input);
                numArgs = countArgs(processedString);
                char *args[numArgs];
                args[numArgs] = NULL;
                splitInput(processedString, args);
                processParallel(args);
                free(processedString);
            }
        }
    }

    if (argc == 2)
    {
        FILE *file = fopen(argv[1], "r");
        if (file == NULL)
        {
            printError();
        }

        char line[256];
        while (fgets(line, sizeof(line), file) != NULL && !feof(file))
        {
            if (compareStrings(line, "exit\n"))
            {
                end = true;
            }
            else if (strspn(line, " \t\n\r\f\v") != strlen(line))
            {
                char *processedString = processString5000(line);
                numArgs = countArgs(processedString);
                char *args[numArgs];
                args[numArgs] = NULL;
                splitInput(processedString, args);
                processParallel(args);
                free(processedString);
            }
        }
        fclose(file);
        end = true;
    }

    free(input);

    return 0;
}