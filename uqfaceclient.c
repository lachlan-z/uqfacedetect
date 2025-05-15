#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h> 
#include <unistd.h>

// Command line args
const char* const replaceArg = "--replacefilename";
const char* const detectArg = "--detectimage";
const char* const outputArg = "--output";

// Error messages
const char* const usageErrorMsg
        = "Usage: ./uqfaceclient port [--replacefilename filename] "
          "[--detectimage filename] [--output filename]\n";
const char* const replaceFileErrorMsg = "uqfaceclient: cannot open the input file \"%s\" for reading\n";
const char* const outputFileErrorMsg = "uqfaceclient: cannot open the output file \"%s\" for writing\n";
const char* const portFailErrorMsg = "uqfaceclient: cannot connect to the server on port \"%s\"\n";
const char* const commErrorMsg = "uqfaceclient: a communication error occurred\n";
const char* const serverErrorMsg = "uqfaceclient: received the following error message: \"%s\"\n"; 

// Error status
typedef enum {
    EXIT_USAGE = 19,
    EXIT_REPLACE_FILE = 14,
    EXIT_OUT_FILE = 11,
    EXIT_PORT_FAIL = 9,
    EXIT_COMM = 13,
    EXIT_SERVER = 12
} ErrorStatus;

typedef struct {
    char* port;
    char* replaceFileName;
    char* detectFileName;
    char* outputFileName;
} CmdLineParams;

void print_cmd(CmdLineParams* params) {
    printf("CmdLineParams:\n");
    printf("    port: %s\n", params->port);
    printf("    replaceFileName: %s\n", params->replaceFileName);
    printf("    detectFileName: %s\n", params->detectFileName);
    printf("    outputFileName: %s\n", params->outputFileName);
}

/* usage_error()
 * ------------------
 * Prints usageErrorMsg to stdout and exits with usage error status.
 * 
 * Errors: usageErrorMsg and exit status.
 */
void usage_error() {
    fprintf(stdout, usageErrorMsg);
    exit(EXIT_USAGE);
}

/* check_empty_value()
 * ------------------ 
 * Checks if a value is empty string or NULL.
 *
 * value: char* value to check.
 *
 * Errors: usageErrorMsg and status if true.
 */
void check_empty_value(char* value) {
    if (strcmp(value, "") == 0 || value == NULL) {
        usage_error();
    }
}

/* parse_command_line()
 * ------------------
 * Parses all command line arguments and initialises the CmdLineParams struct.
 *
 * argc: argument count.
 * argv: argument vector.
 *
 * Returns: Initialised CmdLineParams struct with parsed options
 */
CmdLineParams parse_command_line(int argc, char* argv[])
{
    CmdLineParams params = {0};
     
    argc--;
    argv++;
    
    if (argv[0] && strncmp(argv[0], "--", 2) != 0) {
        check_empty_value(argv[0]);
        params.port = argv[0];
        argc--;
        argv++;
    } else {
        usage_error();
    }

    while (argv[0] && strncmp(argv[0], "--", 2) == 0) {
        if (strcmp(argv[0], replaceArg) == 0 && argv[1] && argv[1][0]) {
            if (params.replaceFileName != NULL) {
                usage_error();
            }
            check_empty_value(argv[1]);
            params.replaceFileName = argv[1];
        } else if (strcmp(argv[0], detectArg) == 0 && argv[1] && argv[1][0]) {
            if (params.detectFileName != NULL) {
                usage_error();
            }
            check_empty_value(argv[1]);
            params.detectFileName = argv[1];
        } else if (strcmp(argv[0], outputArg) == 0 && argv[1] && argv[1][0]) {
            if (params.outputFileName != NULL) {
                usage_error();
            }
            check_empty_value(argv[1]);
            params.outputFileName = argv[1];
        } else {
            usage_error();
        }
        argc -= 2;
        argv += 2;
    }
     
    return params;
}

// determine_input_source
//      From the command line parameters (params) determine whether input
//      is coming from stdin or from a file and return the relevant file handle.
//      The function will not return if a file-opening error occurs.
FILE* determine_input_source(CmdLineParams params)
{
    FILE* stream = stdin;
    if (params.replaceFileName) {
        stream = fopen(params.replaceFileName, "r");
        if (!stream) {
            fprintf(stderr, replaceFileErrorMsg, params.replaceFileName);
            exit(EXIT_REPLACE_FILE);
        }
    }
    return stream;
}

// determine_output_source
//      From the command line parameters (params) determine whether output
//      is going to stdout or to a file and return the relevant file handle.
//      The function will not return if a file-writing error occurs.
FILE* determine_output_source(CmdLineParams params)
{
    FILE* output = stdout;
    if (params.outputFileName) {
        output = fopen(params.outputFileName, "w");
        if (!output) {
            fprintf(stderr, outputFileErrorMsg, params.outputFileName);
            exit(EXIT_OUT_FILE);
        }
    }
    return output;
}

int connect_to_server(char* port) {
   struct addrinfo* ai = 0;
   struct addrinfo hints;
   memset(& hints, 0, sizeof(struct addrinfo));
   hints.ai_family=AF_INET;        // IPv4, for generic could use AF_UNSPEC
   hints.ai_socktype=SOCK_STREAM;
   int err;
   if ((err=getaddrinfo("localhost", port, &hints, &ai))) {
         freeaddrinfo(ai);
         fprintf(stderr, "%s\n", gai_strerror(err));
         return 1;   // could not work out the address
   }

   int fd=socket(AF_INET, SOCK_STREAM, 0); // 0 == use default protocol
   if (connect(fd, ai->ai_addr, sizeof(struct sockaddr))) {
       fprintf(stderr, portFailErrorMsg, port);
       exit(EXIT_PORT_FAIL);
   }

   int fd2=dup(fd);
   FILE* to=fdopen(fd, "w");
   FILE* from=fdopen(fd2, "r");

   fprintf(to, "Hello\n");
   fflush(to);
   fclose(to);

   char buffer[80];
   fgets(buffer, 79, from);
   fclose(from);
   printf("%s", buffer);
   return 0;
}

int main(int argc, char* argv[])
{
    CmdLineParams params = parse_command_line(argc, argv);
    print_cmd(&params);
    FILE* inputStream = determine_input_source(params);
    FILE* outputStream = determine_output_source(params);
    connect_to_server(params.port);
}

