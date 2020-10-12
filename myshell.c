/********************************************************************************************
OS ASSIGNMENT - 1
WRITING YOUR OWN COMMAND SHELL USING OS SYSTEM CALLS

Rishikesh Rachchh
BT18CSE091
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#include <ctype.h>			// isspace()
#include <errno.h>			// error handling (using perror() to print to STDERR)
#include <limits.h>			// PATH_MAX

#define true 1
#define false 0
#define MAX_ARGC 15			// max no of space separated arguments in input (including '&&', '##', etc)
#define bool int

void PRINT_INCORRECT() 		{ printf("Shell: Incorrect command\n"); }
void PRINT_EXITING()	 	{ printf("Exiting shell...\n"); }

/// Print the prompt in format - currentWorkingDirectory$
void printWorkingDirectory()
{
	char cwd[PATH_MAX]; // cwd = current working directory
	if (getcwd(cwd, sizeof(cwd)) != NULL)
			printf("%s$", cwd);
	else
		perror("main() -> getcwd():"); // prints to STDERR
}

// Basically splits entire string into an array with delimiter = separator ( Python equivalent -> s = line.split(separator); argc = len(s) )
// INPUTS - 1. line of input, 2. separator (for splitting), 3. pointer to size_t (that will modify it and store no of args found)
// RETURNS - array of arguments (inlcuding delimiters &&, ##, <, >) 
// SIDE EFFECTS - size_t* argc will contain no of elements in arr after splitting
char** split(char* line, char* separator, size_t* argc)
{
	char** result_array = malloc(MAX_ARGC * sizeof(char *));
	char* parsed;
	*argc = 0;

	char* string = line;
    
    // Splits input into array
    while ((parsed = strsep(&line, separator)) != NULL) 
	{
		// remove leading space (if present)
		while(isspace((unsigned char)*parsed)) parsed++;

		if(*parsed == 0) // if string is empty now (either empty string was parsed, or all chars were spaces that got removed)
			continue;

		// remove trailing space (if present)
		if (parsed[strlen(parsed)-1] == ' ')
			parsed[strlen(parsed)-1] = '\0';
		
		// printf("%s\n", parsed);
        result_array[*argc] = parsed;
        (*argc)++;
    }
    result_array[*argc] = NULL;
    return result_array;
}

// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
// IMPORTANT: This function makes the assumption that more than one type of separator will not be present in the line.
char** parseInput(char* input_line, size_t* no_of_commands, char* separator)
{
	// strip trailing newline (if present) from input line
	input_line[strcspn(input_line, "\n")] = 0;

	if (strpbrk(input_line, "&&") != NULL) 		// search for '&&'
		strcpy(separator, "&&");
	else if (strpbrk(input_line, "##") != NULL) 	// then search for '##'
		strcpy(separator, "##");
	else if (strpbrk(input_line, ">") != NULL) 	// then search for '>'
		strcpy(separator, ">");
	else  // if entire line is just one command, and without redirection (no occurence of "&&", "##" or ">")
		strcpy(separator, "");

	// NOTE: when strsep() (and hence our split() function) is called with separator = "", it returns entire string as a single token (which is what we want here)
	// Hence, we are able to call our split() without checking what the separator actually equals to.
	return split(input_line, separator, no_of_commands);
}

// This function will run multiple commands in parallel
// separator = "&&"
void executeParallelCommands(char** commands, size_t no_of_commands)
{
	size_t argc;
	int index = 0;
	bool flag = false;
	int rc;

	// one fork for each process that needs to be executed in parallel
	do{
		char** cmdargs = split(commands[index], " ", &argc);
		rc = fork();
		if (rc == 0) // child
		{
			flag = false;
			
			// Reverts signals back to original behaviors, so that processes can be killed during execution in children*/ 
    		signal(SIGINT, SIG_DFL);
    		signal(SIGTSTP, SIG_DFL);
			
			int ret = execvp(cmdargs[0], cmdargs);	// cmdargs[0] holds filename of process to run, all elements in cmdargs after that hold args for it	
			// if execvp encounters error
			if (ret == -1)
				PRINT_INCORRECT();
			
			exit(0);
		}
		else if (rc > 1) // parent
			flag = true;
		else
			perror("Error in fork()");

		index++;
	}while(index < no_of_commands && flag);

	// now just wait for all children
	for (int i = 0; i < no_of_commands; i++)
		waitpid(-1, NULL, WUNTRACED);
}

// This function will fork a new process to execute a single command
// ( Also called n times in succession by executeSequentialCommands() )
// "status" parameter gets modified to be either -1 (for failed process) or 0 for successful process
// NOTE: the reason we call chdir() inside the PARENT and NOT the child is because chdir() only changes the working directory for the current process
// separator = ""
void executeCommand(char* command, int* status)
{	
	*status = 0; // if no error then status will be 0

	size_t argc;
	char** cmdargs = split(command, " ", &argc); // array of all args 
	
	// printf("cmdargs[0] = %s, argc = %d\n", cmdargs[0], argc);

	if (strcmp(cmdargs[0], "cd") == 0 && argc <= 2) // if the command is cd, don't fork, just change directory
	{
		// handles the case where no args are passed as well by passing "." by default to chdir
		if( (argc == 1 && chdir(".") != 0) || (argc == 2 && chdir(cmdargs[1]) != 0) ) // only one of the two will be true so it's fine to do this
			perror("executeCommand(): cd");
		return;
	}
	else // command is NOT cd
	{
		pid_t rc = fork();
		if(rc == 0) // it's the child process, so run command
		{
			// Reverts signals back to original behaviour, so that processes can be killed during execution in children 
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			
			int ret = execvp(cmdargs[0], cmdargs);	// cmdargs[0] holds filename of process to run, all elements in cmdargs after that hold args for it	
			// if execvp encounters error
			if (ret == -1)
				PRINT_INCORRECT();
			
			*status = ret;
			exit(0); // terminate child process
		}
		else if (rc > 0 && wait(0) == -1) // it's the parent, so just wait for the child process to terminate
			perror("executeCommand(): wait()");
		else if (rc < 0)	// some error in forking
			perror("executeCommand(): fork()");
	}
}

// This function will run a single command with output redirected to an output file specificed by user
// separator = ">"
void executeCommandRedirection(char** commands, size_t no_of_commands)
{
	// here, commands[0] should hold the actual command with argument, 
	// and commands[1] should hold the file to actually write the output to.

	if (no_of_commands != 2)
	{
		PRINT_INCORRECT();
		// DON'T REPLACE THIS fprintf() WITH perror() because this was just doing validation and we don't know what errno is set to 
		fprintf(stderr, "Error in executeCommandRedirection() -> no of commands = %zu != 2\n", no_of_commands);
		return;
	}

	size_t argc; // argc contains no of elements in cmdargs
	char** cmdargs = split(commands[0], " ", &argc); // cmdargs contains list of all args occuring before '>' separator (including command itself)

	size_t path_argc;
	char** file_path = split(commands[1], " ", &path_argc);

	if(path_argc == 1) // valid case (anything other than this should probably be invalid)
	{
		int fd = open(file_path[0], O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
		
		if (fd < 0) // error in opening file, log to stderr or something
		{
			PRINT_INCORRECT();
			perror("executeCommandRedirection(): couldn't open file \"%s\"");
		}
		else // file opened successfully => redirect stdout to file, then execute command, then restore stdout
		{
			int stdoutCopy = dup(STDOUT_FILENO); // Clone stdout to a new descriptor
			dup2(fd, STDOUT_FILENO); // change stdout to file
			close(fd); // close file

			int ret;
			// Now actually perform the exec, whose output will get redirected to specified file
			executeCommand(commands[0], &ret);	

			// if executeCommand() was successful AND open() didn't cause an error AND dup2() was successful
			if (ret != -1 && fd >= 0 &&	dup2(stdoutCopy, STDOUT_FILENO) != -1)
				close(stdoutCopy); // close the clone
		}
	}
	else	// invalid file path (space separated file path)
	{
		PRINT_INCORRECT();
		fprintf(stderr, "executeCommandRedirection(): path_argc = %zu != 1\n", path_argc);
	}
}

// This function will run multiple commands sequentially
// It is also responsible for calling custom split fn with separator = ' ' for each individual command in char** commands to split each command into separate arguments and do the necesary task
// separator = "##"
void executeSequentialCommands(char** commands, size_t no_of_commands)
{	
	int status;
	for(int i = 0; i < no_of_commands; i++)
		executeCommand(commands[i], &status);
}

size_t __disposable1;
int __disposable2;

int main()
{
	size_t input_size = 100;

	//Used for ignoring both Ctrl+C(SIGINT) and Ctrl+Z(SIGSTP) in the shell 
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);

	while(1)	// This loop will keep the shell running until user exits.
	{
		size_t no_of_commands;		
		char* input_line = malloc(input_size); 	// input buffer
		char* separator = malloc(5); 			// separator between commands (if one or more commands are there in the same line)

		printWorkingDirectory();
		
		getline(&input_line, &input_size, stdin); // get line input, tokenize it, convert to char** result
		
		if ( ! strcmp(input_line, "\n")) // if empty string passed (simply enter was hit in terminal)
			continue;

		char** commands = parseInput(input_line, &no_of_commands, separator);

		if( ! strcmp(split(strdup(commands[0]), " ", &__disposable1)[0], "exit")) // When user uses exit command.
		{
			PRINT_EXITING();
			break;
		}

		if(strcmp(separator, "##") == 0)
			executeSequentialCommands(commands, no_of_commands); 	// For running multiple commands sequentially
		else if(strcmp(separator, "&&") == 0)
			executeParallelCommands(commands, no_of_commands);	 	// For running multiple commands in parallel 
		else if(strcmp(separator, ">") == 0)
			executeCommandRedirection(commands, no_of_commands); 	// For redirecting output of a single command to an output file specified by user
		else
			executeCommand(commands[0], &__disposable2);			// For running just a single command
	}
	
	return 0;
}
