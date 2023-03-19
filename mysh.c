#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZE 100

int main(int argc, char** argv) {
    int fd;

    //check if in Interactive mode
    if (argc == 1) {
	fd = 0;
	char message[] = {"Welcome to my Shell!\nmysh> "};
	   
	if (write(1, message, strlen(message)) == -1) {
		perror("Error writing to STDOUT");
		return EXIT_FAILURE;
	}
    }
    else {
	//check if in batch mode
	if (argc == 2) {
		//open the file given and check if it exists
		fd = open(argv[1], O_RDONLY);
		    
		if (fd == -1) {
			perror("failed to open file");
			return EXIT_SUCCESS;
		}
	}
        else { return EXIT_FAILURE; }

    }

    char *line, **tokens; 
    int numTokens=0; 
    char str[] = {"mysh> "};
    char buffer[BUFSIZE];
    ssize_t bytes = read(fd, buffer, BUFSIZE - 1);

    // read in the bytes until end of file
    while (bytes != 0) {

        buffer[bytes] = '\0';
        line = strtok(buffer, "\n");

        while (line !=  NULL) {
		
            	// Checking if the user wants to exit batch mode
		if (strcmp(line, "exit") == 0) {
               		char exit_message[] = {"mysh: exiting\n"};

			//Checking to see if writing to the commmand line was successful
               		 if (write(1, exit_message, strlen(exit_message)) == -1) {
               		 perror("Error writing to STDOUT");
		     	 return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            	}
	    
		//split each line into to tokens, each token is either '>', '<', '|', or chars seperated by spaces
			
		
			// find the num of tokens in the line
		numTokens = 0; 
	  	for(int i=1; i<strlen(line); i++){
			if(line[i] == ' ' && (line[i-1] != '<' &&  line[i-1] != '>' && line[i-1] != '|' && line[i-1] != ' ')){numTokens++; }
			else{
				if( (line[i] == '<' || line[i] == '>'|| line[i] == '|')  && (line[i-1] != '<' &&  line[i-1] != '>' && line[i-1] != '|' && line[i-1] != ' ')){numTokens += 2;  } 
				else{
					if( (line[i] == '<' || line[i] == '>'|| line[i] == '|')  &&  (line[i-1] == '<' ||  line[i-1] == '>' || line[i-1] == '|' || line[i-1] == ' ') ){numTokens ++; }
					else{
						if(i == (strlen(line)-1) && (line[i] != '<' && line[i] != '>' && line[i] != '|' && line[i] != ' ')){ numTokens++; }; 
					}
				}
			}
		}
	
		
			
		//if the comand is a built in comand,  execute it 

		//if the first token is a path it is an executable file and it shoudl be run using fork() and execv( )
			// check if the following token is '>' or '<' in this case redirect the STDIN or STDOUT
		
		//if the file is not a path check if it is in the listed directories on the p.2 pdf using stat( ), if it is not found are not executable return EXIT_FAILURE; 
		
		

        	line = strtok(NULL, "\n");
        }

        // do something when buffer does not end with new line

        if (argc == 1) {
            if (write(1, str, strlen(str)) == -1) {
                perror("Error reading to STDOUT");
                return EXIT_FAILURE;
            }
        }
        bytes = read(fd, buffer, BUFSIZE - 1);
    }
    return EXIT_SUCCESS;
}

