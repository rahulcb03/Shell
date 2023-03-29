#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <glob.h>

#define BUFSIZE 200 

/**************1************/
//startIndex - The start of the Wild Card token
char * wildCard(char *tokens,  int startIndex ,  int size){
    char **found;
    glob_t gstruct;
    int r;
	
	//endIndex - The start of the token right after the Wild card toekn 
	int endIndex = strlen(&tokens[startIndex]) + startIndex + 1;
	
    //Glob INtialzier
    r = glob(&tokens[startIndex], GLOB_ERR , NULL, &gstruct);
    /* check for errors */
    if( r!=0 )
    {
        if( r==GLOB_NOMATCH )
            perror("No matches\n");
        else
            perror("Some kinda glob error\n");
        
		exit(1);
    }
    
    /* success, output found filenames */
    found = gstruct.gl_pathv;
    
	//Edge Case where the wild card token is the last token 
	int sizeOfWildCard = strlen(&tokens[startIndex]);
	if(endIndex > size){
		for(int i=0; i<sizeOfWildCard + 1; i++){
			tokens[startIndex+i] = '\0';
		}
	}
	//IF the wildcard token is somewher in the middle of the tokens array
	else{
		//change the tokens array to get rid of the old wildCard Token
		for(int i =0; i<size-endIndex; i++){
			tokens[startIndex+i] = tokens[endIndex+i];
		}
	}
	//Find the size to be realloced

	int addedSize = 0;
	int numOfMatches= 0; 
	while(*found){
		addedSize = strlen(*found);
		numOfMatches++;
		found++;
	}
	
	//Account for the size of the wildcard that is being deleted
	addedSize -= sizeOfWildCard;
	//Realloc
	//Have to include the null terminatior
	addedSize += numOfMatches;
	tokens = realloc(tokens, addedSize);

	//Add the matched files to the tokens array 
	//The starting index  would be the start of the wildcard token + the length of whatever is after the wildcard token + 1 cus of the terminator 
	int curr = startIndex + (size-endIndex) + 1;
	while(*found){
		int i=0;
		while(**found){
			tokens[curr] = *found[i];
			i++;
		}
		tokens[strlen(*found) + 1] = '\0';
		curr += strlen(*found) + 2;
	}

    return(0);	
}

/************2*************/




//Changing the Directory 
int changeDir(char* path){
	
	
	//Error if change directory doesn't work  :(
	if(path == NULL || strcmp(path, "~") == 0 ){
		path = getenv("HOME"); 
		if(chdir(path)!=0){
			perror(path);
			return 1;
		}
	}else{
		if(path[0] == '~'){
			char *home = getenv("HOME"); 
			int n = strlen(home); 
			char *p = (char *) malloc(sizeof(char) * (strlen(path) + n ) ); 
			
			int i=0, x=0; 
			for( i=0; i<n; i++){p[i] = home[i];  }
			for( x=1; x<strlen(path); x++){ p[i+x-1] = path[x]; }

			p[x+i-1] = '\0'; 
			
			if(chdir(p)!=0){
				perror(path);
				free(p);
				return 1;
			}
			free(p); 
		}else{
			if(chdir(path)!=0){
				perror(path);
				return 1;
			}
		}
	}
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
	int pathType; //indicates if the path is a Bare name and that it must be freed 
	
	//check if pipe failed
	if(pipe(fds) == -1){
		perror("pipe");
		return 1;
	}

	do{			
		input= NULL;
		output= NULL;
		pathType =0; 

		if(strcmp( &tokens[tokInd], "cd") == 0){
			pathType = 1; 
		}else{
			if(strcmp( &tokens[tokInd], "cd") == 0){
				pathType = 2;
			}else{ 
				//check if the exectuable is a bare name 
				if(strchr(&tokens[tokInd], (int)'/') == NULL){
					path = findBare(&tokens[tokInd]) ; 
					
					//if findBare returns NULL than it was not found
					if(path == NULL){
						char *c = {"Executable is Invalid\n"}; 
						write(1,c , strlen(c) ); 
						return 1; 
					}else{pathType =3; }
				}else{
					path = &tokens[tokInd]; 
					pathType = 4; 
				}	

			}
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
		
		if(pathType ==1){
			if(changeDir(arg[1])) {
				return 1; 
			}
			if(tokInd<size){
				write(fds[0], "0", 1); 
			}else{
				if(output != NULL){
					int f =  open(output, O_WRONLY|O_CREAT|O_TRUNC, 0640 );
					if(f == -1){ 
						perror(output); 
						return 1; 
					}
					write(f, "0", 1); 
					close (f); 
				}
			}
		}
		if(pathType==2){
			char *cwd = getcwd(NULL, 0) ;

			if(cwd == NULL){
				perror("Error"); 
				return 1; 
			}	

			if(tokInd < size){
				write(fds[0] , cwd, strlen(cwd) );
			}else{
				if(output != NULL){
					int d = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0640 );
					if(d==-1){
						perror(output); 
						return 1; 
					}
					write(d, cwd, strlen(cwd) ); 
					close(d); 
				}else{
					write(1, cwd, strlen(cwd) ); 
				}
			}
			free(cwd); 
		}

		if(pathType == 3 || pathType ==4){
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
			if(pathType == 3){free(path);}

			//wait for the child proccess and get the exit status
			int status;
			wait(&status);

			//if the child proccess exits failure than return 1
			if(WIFEXITED(status) ){
				if(WEXITSTATUS(status) == 1){
					return 1; 
				}
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

    char *line, *tokens, *prevLine;
    int counter, tokenIndex, numTokens, newL=0; 
    char str[] = {"mysh> "};
    char buffer[BUFSIZE];
   
   
    ssize_t bytes = read(fd, buffer, BUFSIZE - 1);

    // read in the bytes until end of file
    while (bytes != 0) {
	if(buffer[bytes-1] != '\n'){newL =1;}
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
		if(line[0] == ' ') { counter --; }
				
		for(int x=1; x<strlen(line); x++){
			

			//check if there are multiple spaces
			if(line[x] == ' ' && line[x-1] == ' ' ) 
				counter --; 
			//check if curr char is a token and if last char is a token
			if( (line[x] == '<' || line[x] == '>'|| line[x] == '|')  && line[x-1] != ' ' )
				counter ++; 		
			
			//check if curr char is not a token but the last char was a token
		       	if((line[x] != '<' && line[x] != '>' && line[x] != '|' && line[x] != ' ') && (line[x-1] == '<' || line[x-1] == '>'|| line[x-1] == '|') )	
				counter++;

		}		
		
		//using the counter in the above for loop, malloc an string that can hold each token so that it can be seperated by '\0'
		tokens = (char *)malloc(sizeof(char) * (strlen(line) +counter+1) ); 

		//1b. initailize the new string with the tokens and \0 between each token 
		
		tokenIndex=1; 
		numTokens =0;
		
		if(tokens[0] == ' ') {
			tokenIndex++;
		}else{tokens[0] = line[0]; }

		for(int i=1; i<strlen(line); i++) {

		
			if(line[i] == '*'){
				//if(wCard(tokens, i)){
					write(1, "!", 1); 
				//}
			}
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

		
		// pass the tokens to execute
		if(execute(tokens, strlen(line) + counter, numTokens ) )
			if( argc== 1)
				write(1, "!", 1);
						
		prevLine = line; 
		line = strtok(NULL, "\n");
		
		free(tokens); 
        }

	//if the buffer dosent end on a newline than use lseek to reset to the previous line so the buffer can read the complete line
	if(newL){
		if(lseek(fd,(-1) * strlen(prevLine), SEEK_CUR) == -1){
			perror("lseek"); 
			return EXIT_FAILURE; ; 
		}
		newL = 0; 
	}

        // DO THIS: do something when buffer does not end with new line
	
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

