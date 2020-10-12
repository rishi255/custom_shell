# Custom Shell Written in C

## 1. Basic stuff  
The shell runs an infinite loop (which only exits with the `exit` command) and interactively processes user commands. It prints a prompt that indicate the current working directory followed by `$` character.  

## 2. Changing directory  
The shell supports `cd` command. The command `cd <directoryPath>` changes the working directory to directoryPath and `cd ..` changes the working directory to the parent directory.  

## 3. Incorrect command  
An incorrect command format (which the shell is unable to process) prints the error message `Shell: Incorrect command`. If the shell is able to execute the command, but the execution results in error messages generation, those error messages are displayed to the terminal. An empty command simply causes the shell to display the prompt again without any error messages.  

## 4. Signal handling
The shell is able to handle signals generated from keyboard using `Ctrl + C` and `Ctrl + Z`. When these signals are generated, the shell continues to work and the commands being executed by the shell responds to these signals. The shell itself stops only on receiving the `exit` command.  

## 5. Executing multiple commands  
The shell supports multiple command execution either sequentially or in parallel. The commands separated by `&&` are executed in parallel and the commands separated by `##` are executed sequentially. Also the shell waits for all the commands to be terminated (for parallel and sequential executions, both) before accepting further inputs. Simultaneous use of `&&` and `##` has **not** been handled.  

## 6. Output redirection  
The shell is able to redirect `STDOUT` for the commands using `>` symbol. For example, `ls > infofile` writes the output of `ls` command to `infofile` file instead of writing it on screen. Simultaneous use of multiple commands along with output redirection has **not** been handled.  
