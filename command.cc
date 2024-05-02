#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctime>
#include <time.h>
#include <fstream>
#include <iostream>
#include <cstdlib>

#include "command.h"


char *shell_directory = getenv("HOME/sarah/Downloads/lab2-src");
char LOG_FILE_NAME[] = "/sarah/Downloads/lab2-src/child-log.txt";
char *current_directory_path[128];
int next_directory = 0;
FILE *fp;

void openLogFile() {
	char path_to_log[64];
	strcpy(path_to_log, getenv("HOME"));
	strcat(path_to_log, LOG_FILE_NAME);
	fp = fopen(path_to_log, "a"); // Append mode
}

void closeLogFile() {
	fclose(fp);
}

SimpleCommand::SimpleCommand() // Constructor called when a new SimpleCommand object is created
{
	_numberOfAvailableArguments = 5; // Initializing the _arguments array to hold 5 arguments
	_numberOfArguments = 0; // No arguments have been added yet
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) ); // The _arguments array is dynamically allocated
}

void SimpleCommand::insertArgument( char * argument )// Adding an argument to a SimpleCommand object
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) // Checks if there's enough space in the _arguments array to add the new argument
	{
		// Double the available size of the array
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	// Adds the new argument to the end of the _arguments array
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	// Increments _numberOfArguments by 1
	_numberOfArguments++;
}

Command::Command()// Constructor called when a new Command object is created
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	// The _simpleCommands array is dynamically allocated
	_simpleCommands = (SimpleCommand **) malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );
	// Initializing flags to zero
	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;

}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand )// Adding a SimpleCommand to a Command object
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) // Checks if there's enough space in the _simpleCommands array to add the new simple command
	{
		// Double the available size of the array
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	// Adds the new simple command to the end of the _simpleCommands array
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	// Increments _numberOfSimpleCommands by 1
	_numberOfSimpleCommands++;
}

void Command:: clear() // Free up the memory that was previously allocated for the command and its arguments
{
// Looping through each simple command. For each simple command, looping through its arguments and frees each argument
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
// Free the array of arguments and the simple command itself		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}
// Free the _outFile, _inputFile, and _errFile strings if they have been allocated
	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}
// Setting flags to zero preparing the command to the next use
	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void Command::print()
{ 
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}
void add_dir_to_path(char *directory) 
{
	if (directory == NULL) 
		next_directory = 0; // Stay on the same current path
	else if (strcmp(directory, "..") == 0)
		next_directory--; // Return to previous path
	else
		current_directory_path[next_directory++] = strdup(directory); // Duplicate the directory by taking a pointer to it and returning a pointer to a new dynamically allocated string that is a copy of the original string
}
int changeCurrentDirectory() //cd
{
	int returnValue;
	char *path = Command::_currentSimpleCommand->_arguments[1];
// Retrieving the path to the new directory from the arguments of the current simple command. If a path is provided, it uses that path. Otherwise, it defaults to the shell directory.
	if (path)
		returnValue = chdir(path);
	else
		returnValue = chdir(shell_directory);
// If the directory change was successful or path was not provided, add the new directory to the current_directory_path array using the add_dir_to_path function.
	if (returnValue == 0 || !path)
		add_dir_to_path(path);

	Command::_currentCommand.clear(); // Clear current command
	return returnValue;
}

void removeNewline(char *str, int size)
{
	for (int i = 0; i < size; i++) // Iterates over each character in the string, if a newline character is found, it replaces it with a null character (\0) and then returns
	{
		if (str[i] == '\n')
		{
			str[i] = '\0';
			return;
		}
	}
}

void handleSIGCHLD(int sig_num) // // Signal handler for the SIGCHLD signal, which is sent to a process when one of its child processes terminates
{
	int status;
	wait(&status); // Waiting for the child process to terminate
	openLogFile();
	flockfile(fp); // Locks the file to prevent other processes from writing to it
	time_t TIMER = time(NULL); // Getting the current time
	tm *ptm = localtime((&TIMER)); // Converting current time to local time
	char currentTime[32];
	strcpy(currentTime, asctime(ptm));
	removeNewline(currentTime, 32);
	fprintf(fp, "%s: Child Terminated\n", currentTime);
	funlockfile(fp);
	fclose(fp);
	signal(SIGCHLD, handleSIGCHLD); // Re-registers itself as the handler for the SIGCHLD signal because in some systems, the signal handler is reset to its default setting
}


void Command::execute()
{
	// Checks if there are no simple commands to execute.
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	print();
	// Duplicating the standard input and output file descriptors so that the original file descriptors can be restored later.
	int defaultIn = dup(0);
	int defaultOut = dup(1);

	int ip, op, err;
	// If an error file, input file, or output file is specified, open the file and duplicates its descriptor
	if (_errFile)
	{
		err = open(_errFile, O_WRONLY | O_CREAT, 0777); //  O_WRONLY (write-only) and O_CREAT (create if it doesn't exist) are flags and the file's permissions are set to 0777 (read, write, and execute permissions for all users)
		dup2(err, 2);
	}
	if (_inputFile)
	{
		ip = open(_inputFile, O_RDONLY | O_CREAT, 0777); // O_RDONLY (read-only) flag
	}
	if (_outFile)
	{
		if (!_append)
			op = open(_outFile, O_WRONLY | O_CREAT, 0777);
		else
			op = open(_outFile, O_WRONLY | O_APPEND, 0777);
	}

	int fd[_numberOfSimpleCommands][2];
	// Looping through each simple command and executing it through setting up input and output redirection, forking a new process, and then calling execvp to replace the new process with the simple command
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		pipe(fd[i]);
		if (strcmp(_simpleCommands[i]->_arguments[0], "cd") == 0) // If the simple command is a "cd" command, call changeCurrentDirectory to change the current directory.
		{
			printf("\n");
			if (changeCurrentDirectory() == -1)
				printf("\033[31mError occurred. Make sure the directory you entered is valid\033[0m\n");
			continue;
		}

		if (i == 0)
		{
			if (_inputFile)
			{
				dup2(ip, 0);
				close(ip);
			}
			else
				dup2(defaultIn, 0);
		}
		else
		{
			dup2(fd[i - 1][0], 0);
			close(fd[i - 1][0]);
		}
		if (i == _numberOfSimpleCommands - 1)
		{
			if (_outFile)
				dup2(op, 1);
			else
				dup2(defaultOut, 1);
		}
		else
		{
			dup2(fd[i][1], 1);
			close(fd[i][1]);
		}
		int pid = fork(); // Create a new process, return the child process id to the parent process and zero to the child process
		if (!pid)
		{ // Child
			execvp(_simpleCommands[i]->_arguments[0], &_simpleCommands[i]->_arguments[0]); // Replacing the child process with a new process that will execute the simple command. execvp takes: the name of the program to execute and an array of arguments to pass to the program. 
		}
		else
		{ // Parent
			signal(SIGCHLD, handleSIGCHLD); // Setting handleSIGCHLD as the signal handler for the SIGCHLD signal, which is sent to a process when one of its child processes terminates
			dup2(defaultIn, 0);
			dup2(defaultOut, 1); // duplicating defaultIn to the standard input and defaultOut to the standard output, restoring the original standard input and output to be used by the next command. 
			if (!_background) 
				waitpid(pid, 0, 0);
			// If the command is not running in the background, it waits for the child process to terminate
		}
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation
void catchSIGINT(int sig_num)
{
	signal(SIGINT, catchSIGINT); // SIGINT signal is sent to a process when the user types Ctrl+C. The function will be called whenever the process receives a SIGINT signal.
	Command::_currentCommand.clear(); // Clearing clears the current command
	printf("\r\033[0J");
	// Erasing the current command and the ^C that was printed when the user typed Ctrl+C by Carriage return (\r) and the ANSI escape code for clearing the line (\033[0J)
	Command::_currentCommand.prompt();
	fflush(stdout); // Flushing the standard output so that the new prompt is immediately printed, and it's ready for the user to enter a new command 
}

void Command::prompt()
{
	signal(SIGINT, catchSIGINT);
	printf("myshell>");
	for (int i = 0; i < next_directory; i++)
		printf("%s>", current_directory_path[i]);
	printf(" ");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);
// Used to generate lexical analyzers and parsers in C. It is generated by yacc and is used to parse input according to a grammar specified in a shell.y



int main()
{
	chdir(shell_directory);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

