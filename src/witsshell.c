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

char searchPath[MAX_PATH_LENGTH] = "/bin"; // Initial search path

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
	if (args[1] == NULL)
	{
		// Empty path
		strcpy(searchPath, "");
	}
	else
	{
		strcpy(searchPath, args[1]);
	}
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

// Function to execute a command
void executeCommand(char *args[], bool parallel, const char *outputFile)
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
		if (parallel)
		{
			setpgid(0, 0); // Set new process group for parallel execution
		}

		char fullPath[MAX_PATH_LENGTH + strlen(args[0]) + 2];
		snprintf(fullPath, sizeof(fullPath), "%s/%s", searchPath, args[0]);
		execv(fullPath, args);
		displayError("An error has occurred\n");
		exit(1);
	}
	else if (!parallel)
	{
		// Parent process for sequential execution
		int status;
		waitpid(childPid, &status, 0);
	}

	// Restore the original STDOUT and close the duplicated file descriptor
	dup2(originalStdout, STDOUT_FILENO);
	close(originalStdout);
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

			char *commandArgs[MAX_ARGUMENTS];
			int commandArgCount = 0;
			bool parallel = false;
			const char *outputFile = NULL;
			bool shouldRun = true;
			bool outputDetected = false; // Track if an output file has been detected

			for (int i = 0; i < argCount; i++)
			{
				if (strcmp(args[i], ">") == 0)
				{
					if (i == 0 || i == argCount - 1)
					{
						displayError("An error has occurred\n");
						shouldRun = false;
						break;
					}
					else
					{
						if (outputDetected)
						{
							displayError("An error has occurred\n");
							shouldRun = false;
							break;
						}
						outputFile = args[i + 1]; // Store the output file
						outputDetected = true;
						i++; // Skip the next argument (the output file)
						continue;
					}
				}

				// Check if the argument contains ">" without being an output file
				if (strstr(args[i], ">") != NULL)
				{
					displayError("An error has occurred\n");
					shouldRun = false;
					break;
				}

				if (outputDetected)
				{
					// If an output file has been detected, this argument is an additional output file
					displayError("An error has occurred\n");
					shouldRun = false;
					break;
				}

				commandArgs[commandArgCount] = args[i];
				commandArgCount++;
			}

			commandArgs[commandArgCount] = NULL; // Null-terminate the command argument list

			if (shouldRun)
			{
				executeCommand(commandArgs, parallel, outputFile);
			}
		}
	}

	if (batchMode)
	{
		fclose(inputStream);
	}

	return 0;
}
