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

char path[MAX_PATH_LENGTH] = "/bin"; // Initial search path

// Function to parse and execute the CD command
void execute_cd(char *args[])
{
	if (args[1] == NULL)
	{
		write(STDERR_FILENO, "An error has occurred\n", 23);
	}
	else if (args[2] != NULL)
	{
		write(STDERR_FILENO, "An error has occurred\n", 23);
	}
	else
	{
		if (chdir(args[1]) != 0)
		{
			perror("cd");
		}
	}
}

// Function to set the search path
void set_path(char *args[])
{
	if (args[1] == NULL)
	{
		write(STDERR_FILENO, "An error has occurred\n", 23);
	}
	else
	{
		strcpy(path, args[1]);
	}
}

int main(int argc, char *argv[])
{
	bool batchMode = false;
	FILE *inputStream = stdin;

	// Check if running in batch mode
	if (argc == 2)
	{
		batchMode = true;
		inputStream = fopen(argv[1], "r");
		if (!inputStream)
		{
			perror("fopen");
			exit(1);
		}
	}
	else if (argc > 2)
	{
		write(STDERR_FILENO, "An error has occurred\n", 23);
		exit(1);
	}

	char input[MAX_INPUT_LENGTH];
	char *args[MAX_ARGUMENTS];
	bool redirectOutput = false;
	char *outputFile = NULL;

	while (1)
	{
		if (!batchMode)
		{
			printf("witsshell> ");
			fflush(stdout);
		}

		// Read input from the appropriate stream
		if (fgets(input, sizeof(input), inputStream) == NULL)
		{
			break; // End of file or error
		}

		// Remove trailing newline character
		input[strlen(input) - 1] = '\0';

		// Parse the input into arguments
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
			// Check for built-in commands
			if (strcmp(args[0], "exit") == 0)
			{
				if (argCount > 1)
				{
					write(STDERR_FILENO, "An error has occurred\n", 23);
				}
				else
				{
					exit(0); // Exit the shell
				}
			}
			else if (strcmp(args[0], "cd") == 0)
			{
				execute_cd(args); // Execute CD command
				continue;		  // Continue to the next iteration of the loop
			}
			else if (strcmp(args[0], "path") == 0)
			{
				set_path(args); // Set the search path
				continue;		// Continue to the next iteration of the loop
			}
			else if (strcmp(args[0], "clear") == 0)
			{
				system("clear"); // Clear the screen
				continue;		 // Continue to the next iteration of the loop
			}
		}

		// Check for output redirection
		redirectOutput = false;
		for (int i = argCount - 1; i > 0; i--)
		{
			if (strcmp(args[i], ">") == 0)
			{
				if (i + 1 < argCount)
				{
					redirectOutput = true;
					args[i] = NULL;			  // Remove the ">" argument
					outputFile = args[i + 1]; // Store the output file
					break;
				}
				else
				{
					write(STDERR_FILENO, "An error has occurred\n", 23);
					continue;
				}
			}
		}

		// Check for parallel execution
		bool parallel = false;
		for (int i = 0; i < argCount; i++)
		{
			if (strcmp(args[i], "&") == 0)
			{
				parallel = true;
				args[i] = NULL; // Remove the "&" argument
				break;
			}
		}

		if (parallel)
		{
			// Fork a child process for parallel execution
			pid_t child_pid = fork();
			if (child_pid == -1)
			{
				perror("fork");
				exit(1);
			}
			else if (child_pid == 0)
			{
				// Child process
				if (redirectOutput)
				{
					int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
					if (fd == -1)
					{
						perror("open");
						exit(1);
					}
					dup2(fd, STDOUT_FILENO);
					dup2(fd, STDERR_FILENO);
					close(fd);
				}

				char full_path[MAX_PATH_LENGTH + strlen(args[0]) + 2];
				snprintf(full_path, sizeof(full_path), "%s/%s", path, args[0]);
				execv(full_path, args);
				perror("execv");
				exit(1);
			}
		}
		else
		{
			// Fork a child process for sequential execution
			pid_t child_pid = fork();
			if (child_pid == -1)
			{
				perror("fork");
				exit(1);
			}
			else if (child_pid == 0)
			{
				// Child process
				if (redirectOutput)
				{
					int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
					if (fd == -1)
					{
						perror("open");
						exit(1);
					}
					dup2(fd, STDOUT_FILENO);
					dup2(fd, STDERR_FILENO);
					close(fd);
				}

				char full_path[MAX_PATH_LENGTH + strlen(args[0]) + 2];
				snprintf(full_path, sizeof(full_path), "%s/%s", path, args[0]);
				execv(full_path, args);
				perror("execv");
				exit(1);
			}
			else
			{
				// Parent process
				int status;
				waitpid(child_pid, &status, 0);
			}
		}
	}

	if (batchMode)
	{
		fclose(inputStream);
	}

	return 0;
}
