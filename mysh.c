#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 200 

char * wildCard(char *tokens,  int startIndex ,  int *size){
    char **found;
    glob_t gstruct;
    int r;


    //Glob INtialzier
    r = glob(&tokens[startIndex], GLOB_ERR , NULL, &gstruct);
    /* check for errors */
    if( r!=0 )
    { 
		return NULL;
    }

    /* success, output found filenames */
    found = gstruct.gl_pathv;

	int numFiles=0; 
	int fileSize =0; 

	while(found[numFiles]!= NULL){
		fileSize += strlen(found[numFiles])+1; 
		numFiles++;
		
	}

	numFiles--; 
	
	//checks to make sure the executable can only have one match
	if(numFiles >1 && startIndex==0){
		return NULL; 
	}

	int newSize = *size - strlen(&tokens[startIndex])-1+fileSize; 

	tokens = realloc(tokens, newSize); 

	//moves everything down the tokens array 
	int x = newSize -1; 
	for(int i =*size-1; i>(startIndex + strlen(&tokens[startIndex])); i--){
		tokens[x] = tokens[i];
		x--; 
	}

	int z=0; 
	int index = startIndex; 

	while(found[z]!=NULL){
		for(int c=0; c<strlen(found[z]); c++){
			tokens[index] = found[z][c];
			index++; 
		}
		tokens[index] = '\0';
		index++;
		z++; 
	}

	
	globfree(&gstruct); 
	
	*size = newSize; 
	return tokens; 
}


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
	int doExit =0; 
	
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
			if(strcmp( &tokens[tokInd], "pwd") == 0){
				pathType = 2;
			}else{ 
				if(strcmp(&tokens[tokInd], "exit") ==0){
					pathType=5;
				}else{
				//check if the exectuable is a bare name 
					if(strchr(&tokens[tokInd], (int)'/') == NULL){
						path = findBare(&tokens[tokInd]) ; 
					
					//if findBare returns NULL than it was not found
						if(path == NULL){
							char *c = {"Executable is Invalid\n"}; 
							write(1,c , strlen(c) ); 
							free(arg);
							return 1; 
						}else{pathType =3; }
					}else{
						path = &tokens[tokInd]; 
						pathType = 4; 
					}	
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
				free(arg);
				return 1; 
			}
			if(tokInd<size){
				write(fds[1], "0", 1); 
			}else{
				if(output != NULL){
					int f =  open(output, O_WRONLY|O_CREAT|O_TRUNC, 0640 );
					if(f == -1){ 
						perror(output); 
						free(arg);
						return 1; 
					}
					write(f, "0\n", 2); 

					close (f); 
				}
			}
		}
		if(pathType==2){
			char *cwd = getcwd(NULL, 0) ;

			if(cwd == NULL){
				perror("Error"); 
				free(arg);
				return 1; 
			}	

			if(tokInd < size){			
				write(fds[1] , cwd, strlen(cwd) );
			}else{
				if(output != NULL){
					int d = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0640 );
					if(d==-1){
						perror(output); 
						free(arg);
						return 1; 
					}
					write(d, cwd, strlen(cwd) ); 
					write(d, "\n", 1); 
					close(d); 
				}else{
					write(1, cwd, strlen(cwd) ); 
					write(1, "\n", 1); 
				}
			}
			free(cwd); 
		}

		if(pathType == 3 || pathType ==4){
			//use fork to create child proccess
			int pid = fork();

			if(pid ==-1) {
				perror("fork"); 
				free(arg);
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
					free(arg);
					return 1; 
				}
			}
		
		}
		if(pathType == 5){
			if(tokInd<size){
				write(fds[1], "0", 1);
			}
			doExit =1;
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

	if(pathType == 4 && output == NULL ){
		write(1,"\n", 1); 
	}

	if(doExit ==1){
		free(arg);
		return -1; 
	}
	
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
    int counter, tokenIndex, newL=0; 
    char str[] = {"mysh> "};
    char buffer[BUFSIZE];
    int errorFlag; 
   
   
    ssize_t bytes = read(fd, buffer, BUFSIZE - 1);

    // read in the bytes until end of file
    while (bytes != 0) {
		if(buffer[bytes-1] != '\n'){newL =1;}
	// add \0 to the end so we can use strtok( ) to split the buffer into serpatelines
        buffer[bytes] = '\0';
        line = strtok(buffer, "\n");
	
	//traverse through the lines in the buffer
        while (line !=  NULL) {
			          
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
		
			if(tokens[0] == ' ') {
				tokenIndex++;
			}else{tokens[0] = line[0]; }

			for(int i=1; i<strlen(line); i++) {

		
				if(line[i] == ' ' && line[i-1] != ' '){
					tokens[tokenIndex] = '\0'; 
					tokenIndex++;
				}else{
					if( (line[i] == '<' || line[i] == '>'|| line[i] == '|')  && line[i-1] != ' '){
						tokens[tokenIndex] = '\0';
						tokenIndex++; 
						tokens[tokenIndex] = line[i]; 
						tokenIndex++;
					}else{
						if((line[i] != '<' && line[i] != '>' && line[i] != '|' && line[i] != ' ') && (line[i-1] == '<' || line[i-1] == '>'|| line[i-1] == '|') ){
							tokens[tokenIndex] = '\0'; 
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
			errorFlag = 0; 
			
			int i=0; 
			 
			int size = strlen(line)+counter+1; 

			while(i<size){
				if(strchr(&tokens[i], '*') != NULL ){

					tokens = wildCard(tokens, i, &size); 
					
					if(tokens == NULL ){
						errorFlag = 1;
						break; 
					}
				}
				i+=strlen(&tokens[i]) +1;
			}

			if(tokens!=NULL){

			

				int x=0; 
				int numTokens=0; 
				while(x<size){
					numTokens++; 
					x+=strlen(&tokens[x]) +1;
				}
			
		
			// pass the tokens to execute

				int exc = execute(tokens, size, numTokens );
				char end[] ={"mysh: exiting\n"};
				if(exc ==1)
					errorFlag =1; 
				if(exc ==-1){
			
					write(1, end, strlen(end) ); 
					free(tokens);
					return EXIT_SUCCESS; 

				}
			}else{
				if(argc ==1){
					write(1, "Invalid Input\n", 14); 
				}
			}
				
			prevLine = line; 
			line = strtok(NULL, "\n");
			if( tokens != NULL)
				free(tokens); 
       	}

	//if the buffer dosent end on a newline than use lseek to reset to the previous line so the buffer can read the complete line
		if(newL){
			if(lseek(fd,(-1) * strlen(prevLine), SEEK_CUR) == -1){
				perror("lseek"); 
				return EXIT_FAILURE; 
			}
			newL = 0; 
		}

	
       	if (argc == 1) {
			if(errorFlag){
				write(1, "!", 1); 
			}
          	if (write(1, str, strlen(str)) == -1) {
          	      perror("Error reading to STDOUT");
         	       return EXIT_FAILURE;
         	}
		}
        bytes = read(fd, buffer, BUFSIZE - 1);
    }
	return EXIT_SUCCESS;
}
 