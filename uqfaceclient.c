#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Command line args
const char* const replaceArg = "--replacefilename";
const char* const detectArg = "--detectimage";
const char* const outputArg = "--output";

// Error messages
const char* const usageErrorMsg
        = "Usage: ./uqfaceclient port [--replacefilename filename] "
          "[--detectimage filename] [--output filename]\n";

// Error status
typedef enum {
    EXIT_USAGE = 19
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

int main(int argc, char* argv[])
{
    CmdLineParams params = parse_command_line(argc, argv);
    print_cmd(&params);
}

