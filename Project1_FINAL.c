// EECE 315: Project 1
// Linux Shell (Command Interpreter)
//
// Date: 27/02/2013
//
// Lab Section: L2A
// Group: A6
//
// Group members:
// Alex Sismanis
// Amanbir Singh
// Christopher Tan
// Hooman Shariati
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>


#define ARGS_MAX 1024
#define MAX_PATH_LENGTH 2048
#define HOST_NAME_MAX 100
#define MAX_CHARS_ACCEPT 100

//Functions
int parsePath(char **);
char *findPath(char **, char **);
int createModEnviro(char *enviro, char *change);
void deleteEnviro(char *enviro);
int setPathEnviro(char *change);
void getPathEnviro(char **dir);

int count; //global variable count to save position


struct command_t {
	char *name;
	int argc;
	char *argv[];
};

void fatal( char *message ){
	printf("Error: %s\n", message );
	exit(1);
}

void execute_child (char* commandName, char** commandArray, int ac) {
	// Create a process to execute the command
	pid_t pid;
	int ret_status;

	pid = fork();

	//Child process
	if (pid == 0) {
		commandArray[ac] = 0;   //unsets the last elements of argv array for using execv
                if (execv(commandName, commandArray) < 0) {
			fatal("exec failed");
			exit(1);
		}
	}
	else if (pid > 0) {
		// Parent waits until child finishes executing command
		pid=waitpid(pid, &ret_status, 0);  //waits only for the foreground childs
	}
	else {
		printf("FATAL error: fork returned error code, no child produced. \n");
		exit(3);
	}

}

void background (char* commandName, char** commandArray, int ac) {
	// Create a process to execute the command
	pid_t pid;
	int ret_status;

	pid = fork();

	if (pid == 0) {
                commandArray[ac] = 0; //unsets the last elements of argv array for using execv
		if (execv(commandName, commandArray) < 0) {
			fatal("exec failed");
			exit(1);
		}
	}
	else if (pid > 0) {
		printf("PID\n %d\n",(int)pid);
	}
	else {
		printf("FATAL error: fork returned error code, no child produced. \n");
		exit(3);
	}

}

//get envro var of path
void getPathEnviro(char **dir){ //pass into this the samplepath that we generated
	int i;
	for(i = 0; i < count; i++){
        printf("%s\n", dir[i]);
    }
}

//set enviro of PATH function
int setPathEnviro(char *change){ //argv[1] will be what the user wants it changed too

    char *pathEnviroVar;
    char *spacer = ":";

    pathEnviroVar = (char *) getenv ("PATH"); //get environment variable from PATH
    strcat(pathEnviroVar, spacer); //adds a : in between dirs
    strcat(pathEnviroVar, change); //adds the new dir for PATH enviro var

    return setenv("PATH", pathEnviroVar, 1);

}

int createModEnviro(char *enviro, char *change){ //argv 1 is the enviro to change and argv 2 would be what to change it to

    return setenv( enviro, change, 1); //sets argv 1 to agrv 2

}


void deleteEnviro(char *enviro){ //argv 1 is the enviro to delete

     unsetenv( enviro); //deletes enviro variable
    return;

}

int parsePath(char *dirs[])
{

	// This function reads the PATH variable for this environment, then builds an
    // array, dirs[], of the directories in PATH
	char *pathEnviroVar;
	char *myPath;
	char *pointer;
    int length;
	int i;
	int position = 0;


	for( i=0; i<ARGS_MAX; i++) {
		dirs[i] = NULL; //initialize to null
	}

    pathEnviroVar = (char *) getenv ("PATH"); //get environment variable from PATH
	myPath = (char *) malloc(strlen(pathEnviroVar) + 1); //allocate myPath to be length of Path enviro Variable + 1

    strcpy(myPath, pathEnviroVar); //copy pathEnviroVar into myPath


    char strng[MAX_PATH_LENGTH];
  	int length2 = 0;

  	for(pointer = myPath; *pointer != '\0'; pointer++) //loop through string til end: \0
  	{
        length = strlen(pointer); //get string length of pointer
  		strncpy(strng, pointer, length); //copy current string into new strng[] variable
        pointer = strchr(pointer, ':'); //strchr sets pointer to the first instance of :


        //jump into this loop if no more instances of :
        if (pointer==NULL){ //if pointer is null reinitialize
            pointer = myPath;
    		length = strlen(pointer);
            pointer = strrchr(pointer, ':'); //finds and points to last occurence of :
            pointer++;

            strcpy(strng, pointer);
            dirs[position]= (char *) malloc(strlen(strng)+1);
            strcpy(dirs[position],strng);
            position++;
            count = position;


            break; //jump out of for
        }

    	length2 = strlen(pointer); //finds the length of the string from pointer pointing to the first :

    	strng[length-length2] = '\0'; // adds a termination char to the strng to signal end of one pathname
        dirs[position]= (char *) malloc(strlen(strng)+1); //adds the memory for strng
        strcpy(dirs[position],strng); //copies strng into the dirs
        position++;
	}

    return 0;
}



char *findPath(char **argv, char **dir)
{
    char *found;
    char name[MAX_PATH_LENGTH];


    if( *argv[0]== '/') //checks if the file name is already the root directory
    {
        found= *argv;
        return found;       //if it is return argv
    }
	int i;
    for( i=0; i<count; i++)         //otherwise check each
    {
        strcpy(name, dir[i]);
        strcat(name, "/"); //adds / to end of name
        strcat(name, argv[0]); // adds the argv[0] to the end of name

        if( access(name, F_OK|X_OK) != -1) //checks if the directory exists and if we have acess
        {
            found = (char*) malloc (strlen(name)+1);    //allocates memory length of name to found
            strcpy(found, name); // copies name into found
            return found;   // returns the found path
        }
    }
    return NULL;    //returns nothing if path cant be found
}




int main () {

	//VARIABLES
	int i = 0;
	struct command_t *command; // Shell initialization
	command = malloc(sizeof(struct command_t));
	command->name = malloc(sizeof(char));
	command->argv[0] = malloc(sizeof(char));
	command->argc = 0;

	uid_t uid = getuid();
	struct passwd *p = getpwuid(uid);  // Check for NULL!

    char* pathsample[ARGS_MAX];
    parsePath(pathsample); // Get directory paths from PATH

	//Reminder for user to enter endc at the end of commands. (i.e. "ls -l endc")
	printf("\nMake sure to end all Unix commands with the string endc \n \n");
	
	// Main loop
	while(1) {

		// Print the prompt string (i.e. yourcomputername~$ or something, see project 1 document)
		printf("%s~$", p->pw_name);

		// Read the command line and parse it
		scanf("%s", command->name);

		//Copy the command->name to command->argv[0]
		//command->argv[0] = malloc(sizeof(char));
		command->argv[0] = command->name;

		//Help message
		if (strcmp(command->name, "help") == 0) {
			printf("\nMake sure to end all Unix commands with the string endc \n \n");
			continue;
		}


		//reset values for loop
		i = 0;
		command->argc = 0;
		while((strcmp(command->argv[i], "endc")) && (strcmp(command->argv[i], "&"))){
			//start counting how many arguments in the array
			command->argc++;
			i++;
			command->argv[i] = malloc(sizeof(char) * 128);
			scanf("%s", command->argv[i]);
		}


		//exit condition
		if (strcmp (command->name,"exit") == 0) {
			exit(0);
		}

		//change directory function
		else if (strcmp (command->name, "cd") == 0) {
            if(command->argc==1)
            	chdir(getenv("HOME"));
			else
            {
				char compare[MAX_CHARS_ACCEPT];
				char* toCompare;
				int index = 2;
				//If there are double quotes in command->argv[1]
				toCompare = strchr(command->argv[1], '"');
				if (toCompare - command->argv[1] == 0)
				{
					strcpy(compare, command->argv[1]);
					//function that helps parse spaced directories
					while (strcmp(command->argv[index],"endc"))
					{
						strcat(compare, " ");
						strcat(compare, command->argv[index]);
						index++;
					}
					//Removes double quotes in the front and back
					memmove(compare,compare+1,strlen(compare));
					toCompare = compare;
					toCompare[strlen(toCompare)-1] = 0;
					strcpy(command->argv[1],toCompare);
				}
				const char * path = command->argv[1];
				if (chdir(path) == -1)
                {
					printf("Directory change failed \n");
			    }
            }
		}

		//continue prompting even with no command
		else if (command->argc == 0) {
			continue;
		}

		//checks system path
		else if (strcmp(command->name, "checkpath") == 0) {
			getPathEnviro(pathsample);
		}
		
		//modifies system path
		else if (strcmp(command->name, "setpath") == 0) {
			setPathEnviro(command->argv[1]);
		}

		//creates an environmental variable
		else if (strcmp(command->name, "createenv") == 0) {
			createModEnviro(command->argv[1], command->argv[2]);
		}

		//deletes an environmental variable
		else if (strcmp(command->name, "deleteenv") == 0) {
			deleteEnviro(command->argv[1]);
		}


		//runs programs given directory
		else if (command->name[0] == '/') {
		       if (strcmp (command->argv[command->argc], "endc"))

                              background(command->name,command->argv,command->argc);
                        else

			//Execute the child
			      execute_child(command->name, command->argv,command->argc);
		}

		//runs programs in current directory
		else if (command->name[0] == '.') {
	               if (strcmp (command->argv[command->argc], "endc"))

                              background(command->name,command->argv,command->argc);
                        else

			//Execute the child
			    execute_child(command->name, command->argv,command->argc);
		}

		//running processes in the background
		//else if (strcmp (command->argv[command->argc - 1], "&"))
		//run background process

		else {
			// Find the full pathname for the file
			command->name = findPath(command->argv, pathsample);
                        if (strcmp (command->argv[command->argc], "endc"))

                              background(command->name,command->argv,command->argc);
                        else

			//Execute the child
			execute_child(command->name, command->argv,command->argc);
		}

	}

	return(0);
}



