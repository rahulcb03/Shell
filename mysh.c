#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define BUFSIZE 100

//Changing the Directory 
int changeDir(char* path){
	
	//Error if change directory doesn't work  :(
	if(chdir(path)!=0){
		perror(path);
		return 1;
	}

	//If Everything goes as planned return success muahahahahahha
	return 0; 
}

//Printing the Current Directory
int printCurrDir(){
	//Get all the paths in the current directory
	char *buffer = getcwd(NULL, 0);

	//Checking to see if there was an error with getcwd
	if(buffer == NULL){
		perror("Error getting current working directory"); 
		return 1;
	}

	//Writing to the stdout and checking if theres any issues with the writing
	write(1, buffer, strlen(buffer));
	char c='\n'; 
	write(1, &c, 1); 
			
	//Free buffer and return success
	free(buffer);
	return 0;

}

//takes in a bare name and should return path to that executable or NULL if not found
//if it doesnt return NULL than the return value must be freed
char * findBare(char *token){
	struct stat st;

	char *dirs[] =  {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin","/bin"};

	for(int i=0; i<6; i++){

		//stores the new path in path
		char *path = (char *)malloc(sizeof(char) * (strlen(dirs[i]) + strlen(token) +2) ); 
		
		int x=0, y=0; 

		//build the new path 
		for( x=0; x<strlen(dirs[i]); x++){ path[x] = dirs[i][x];}
		path[x] = '/'; 
		for( y=0; y<strlen(token) ; y++){ path[y+x+1] = token[y];}
		path[y+x+1] = '\0';  
		
		//see if it exists and return it
		if(stat(path, &st) == 0){
			return path; 
		}else{
			free(path); 
		}

	}

	return NULL; 
	
}

int execute(char *tokens, int size, int nTok){
	
	char *path; //stores the exectable path
	char *input=NULL, *output=NULL; //stores the input and output files
	char ** arg = (char **) malloc(sizeof(char *) * (nTok +1) ); //stores arguments for executable
	int tokInd=0 ; //used to index throuhgh tokens
       	int num=0; //keeps track of the iterations
	int fds[2]; //pipe FD
	int isBare; //indicates if the path is a Bare name and that it must be freed 
	
	//check if pipe failed
	if(pipe(fds) == -1){
		perror("pipe");
		return 1;
	}

	do{
		input= NULL;
		output= NULL;
		isBare =0; 

		//check if the exectuable is a bare name 
		if(strchr(&tokens[tokInd], (int)'/') == NULL){
			path = findBare(&tokens[tokInd]) ; 
			
			//if findBare returns NULL than it was not found
			if(path == NULL){
				char *c = {"Executable is Invalid\n"}; 
				write(1,c , strlen(c) ); 
				return 1; 
			}else{isBare =1; }
		}else{
			path = &tokens[tokInd]; 
		}	
	
		//build the argument list 
		char *prevTok  = &tokens[tokInd]; 
		arg[0] = &tokens[tokInd]; 
		tokInd += strlen(&tokens[tokInd]) +1; 
		int argInd =1; 
		
		//traverse until the end of command or until pipe
		//if prev is < or > than the next token is a redirect file 
		//else it is an argument 
		while(tokInd<size && strcmp(&tokens[tokInd], "|") != 0 ){
			if(strcmp(prevTok, "<") != 0 && strcmp(prevTok, ">") != 0 && strcmp(&tokens[tokInd], "<") != 0 && strcmp(&tokens[tokInd], ">") != 0  ){
				arg[argInd] = &tokens[tokInd]; 
				argInd++;
			}else{
				if(strcmp(prevTok,"<") ==0){
					input = &tokens[tokInd];
				}
				if(strcmp(prevTok, ">") ==0){
					output = &tokens[tokInd];
				}
			
			}

			prevTok = &tokens[tokInd]; 
			tokInd +=strlen(&tokens[tokInd]) +1;
		}
		arg[argInd] = NULL; 
		
		//use fork to create child proccess
		int pid = fork();

		if(pid ==-1) {
			perror("fork"); 
			return 1;
	       	}

		if( pid ==0){
		//currently in child process
			//check if the input and output is set and if so open the file and redirect 
			if( input != NULL) {
				int inFD = open(input, O_RDONLY );
				if(inFD == -1) {
					perror(input);
					exit(1); 
				}
				if(dup2(inFD , STDIN_FILENO) == -1){
					perror( "dup2"); 
					exit(1); 
				}
				close(inFD);
			}
			if(output != NULL){
				int outFD = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0640 );
				if( outFD == -1 ){
					perror(output);
					exit(1); 
				}
				if(dup2(outFD , STDOUT_FILENO) == -1){
					perror( "dup2");
					exit(1); 
				}
				close(outFD);
			}
			
			//check if there is a pipe and redirect using pipe
			if(tokInd<size && strcmp(&tokens[tokInd] , "|") ==0 && num ==0 ) {
				
				if(dup2(fds[1], STDOUT_FILENO) == -1){
					perror("pipe"); 
					exit(1);	
				}			
			}

			//if this is the second iteration than there was a pipe so set the input to the pipe fds
			if(num ==1){
				if(dup2(fds[0], STDIN_FILENO) == -1){
					perror("pipe"); 
					exit(1);	
				}		
			}

			//excute the executable with the arguments given by the command 
			execv(path,arg ); 

			//if here than there was an error executing 
			perror(path); 
			exit(1);
		
		}
		if(isBare){free(path);}

		//wait for the child proccess and get the exit status
		int status;
		wait(&status);

		//if the child proccess exits failure than return 1
		if(WIFEXITED(status) ){
			if(WEXITSTATUS(status) == 1){
				return 1; 
			}
		}
		
		//close the input end of the pipe
		if(num ==1){close(fds[0]);}

		//close output end of the pipe and iterate over the '|'
		if(tokInd<size){
			close(fds[1]);
			num =1;
			tokInd += strlen(&tokens[tokInd]) +1; 
		}
		
	}while(tokInd<size);

	
	free(arg);
	return 0;

}



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
			perror(argv[1]);
			return EXIT_SUCCESS;
		}
	}
        else { return EXIT_FAILURE; }

    }

    char *line, *tokens; 
    int counter, tokenIndex, numTokens; 
    char str[] = {"mysh> "};
    char buffer[BUFSIZE];
    ssize_t bytes = read(fd, buffer, BUFSIZE - 1);

    // read in the bytes until end of file
    while (bytes != 0) {
	// add \0 to the end so we can use strtok( ) to split the buffer into serpatelines
        buffer[bytes] = '\0';
        line = strtok(buffer, "\n");
	
	//traverse through the lines in the buffer
        while (line !=  NULL) {
			
            	// Checking if the user wants to exit
		if (strcmp(line, "exit") == 0) {
               		char exit_message[] = {"mysh: exiting\n"};

			//Checking to see if writing to the commmand line was successful
               		 if (write(1, exit_message, strlen(exit_message)) == -1) {
               			 perror("Error writing to STDOUT");
		     		 return EXIT_FAILURE;
               		 }
               		 return EXIT_SUCCESS;
            	}
	    
		//1. split each line into to tokens, each token is either '>', '<', '|', or chars seperated by spaces
		//plan: create new String that holds the tokens, the tokens will be seperated by \0 so they can be used as spereate strings
			
		counter =0; 
		//1a. find how many additional spaces are needed to include \0 for each token
		for(int x=1; x<strlen(line); x++){
			//check if there are multiple spaces
			if(line[x] == ' ' && line[x-1] == ' ') 
				counter --; 
			//check if curr char is a token and if last char is a token
			if( (line[x] == '<' || line[x] == '>'|| line[x] == '|')  && (line[x-1] != '<' &&  line[x-1] != '>' && line[x-1] != '|' && line[x-1] != ' '))
				counter ++; 		
			
			//check if curr char is not a token but the last char was a token
		       	if((line[x] != '<' && line[x] != '>' && line[x] != '|' && line[x] != ' ') && (line[x-1] == '<' || line[x-1] == '>'|| line[x-1] == '|') )	
				counter++;
		}		
		
		//using the counter in the above for loop, malloc an string that can hold each token so that it can be seperated by '\0'
		tokens = (char *)malloc(sizeof(char) * (strlen(line) +counter+1) ); 

		//1b. initailize the new string with the tokens and \0 between each token 
		tokens[0] = line[0];	
		tokenIndex=1; 
		numTokens =0; 
		for(int i=1; i<strlen(line); i++) {
			if(line[i] == ' ' && line[i-1] != ' '){
				tokens[tokenIndex] = '\0'; 
				tokenIndex++;
				numTokens++; 
			}
			else{
				if( (line[i] == '<' || line[i] == '>'|| line[i] == '|')  && line[i-1] != ' '){
					tokens[tokenIndex] = '\0';
					tokenIndex++; 
					numTokens++; 
					tokens[tokenIndex] = line[i]; 
					tokenIndex++;
				}
				else{
					if((line[i] != '<' && line[i] != '>' && line[i] != '|' && line[i] != ' ') && (line[i-1] == '<' || line[i-1] == '>'|| line[i-1] == '|') ){
						tokens[tokenIndex] = '\0'; 
						numTokens++; 
						tokenIndex++; 
					}
					if(line[i] != ' '){	
						tokens[tokenIndex] = line[i] ;
						tokenIndex++; 
					}
					
				}
			}
		}
		tokens[strlen(line)+counter ] = '\0';
		numTokens++; 
		
		char c; 
		//check if the first comand is a built in comand (cd,pwd )
		if(strcmp( tokens, "cd") == 0){
			if(changeDir(&tokens[3]) ){
				if(argc == 1){
					c = '!'; 
					write(1, &c, 1);
				}
			}
		}
		else{
			if(strcmp(tokens, "pwd") ==0){
				if(printCurrDir( ) ){
					if(argc == 1){
						c = '!'; 
						write(1, &c, 1);
					}
				}
			}
			else{
				// pass the tokens to execute
				if(execute(tokens, strlen(line) + counter, numTokens ) )
					write(1, "!", 1);
			}										
		}			

        	line = strtok(NULL, "\n");
        }

        // DO THIS: do something when buffer does not end with new line
	free(tokens); 
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

