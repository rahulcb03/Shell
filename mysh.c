#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZE 100

int main(int argc, char** argv) {
    int fd;
    if (argc == 1) {
        fd = 0;
        char message[] = {"Welcome to my Shell!\nmysh> "};
        if (write(1, message, strlen(message)) == -1) {
            perror("Error writing to STDOUT");
            return EXIT_FAILURE;
        }
    }
    else {
        if (argc == 2) {
            // run in Batch mode

            // open the file given and check if it exists
            fd = open(argv[1], O_RDONLY);
            if (fd == -1) {
                perror("failed to open file");
				return EXIT_SUCCESS;
            }
        }
        else {
            return EXIT_FAILURE;
        }

    }

    char* token;
    char str[] = {"mysh> "};
    char buffer[BUFSIZE];
    ssize_t bytes = read(fd, buffer, BUFSIZE - 1);

    // read in the bytes until end of file
    while (bytes != 0) {

        buffer[bytes] = '\0';
        token = strtok(buffer, "\n");

        while (token != NULL) {
            // Checking if the user wants to exit batch mode
            if (strcmp(token, "exit") == 0) {
                char exit_message[] = {"mysh: exiting\n"};
				//Checking to see if writing to the commmand line was successful
                if (write(1, exit_message, strlen(exit_message)) == -1) {
                    perror("Error writing to STDOUT");
					return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            }
			
			//Other shinaningnasn with the token 
            token = strtok(NULL, "\n");
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

