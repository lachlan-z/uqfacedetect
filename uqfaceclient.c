#include <stdio.h>
#include <string.h>

// Command line args
const char* const replaceArg = "--replacefilename";
const char* const detectArg = "--detectimage";
const char* const outputArg = "--output";

// Error messages
const char* const usageErrorMsg
        = "Usage: ./uqfaceclient port [--replacefilename filename] "
          "[--detectimage filename] [--output filename]";

// Error status
typedef enum {
    EXIT_USAGE = 19
} ErrorStatus;

typedef struct {
    char* replaceFileName;
    char* detectFileName;
    char* outputFileName;
} CmdLineParams;

void print_cmd(CmdLineParams* params) {
    printf("CmdLineParams:\n");
    printf("    replaceFileName: %s\n", params->replaceFileName);
    printf("    detectFileName: %s\n", params->detectFileName);
    printf("    outputFileName: %s\n", params->outputFileName);
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

    while (argv[0] && strncmp(argv[0], "--", 2) == 0) {
        if (strcmp(argv[0], replaceArg) == 0 && argv[1] && argv[1][0]) {
            params.replaceFileName = argv[1];
        } else if (strcmp(argv[0], detectArg) == 0 && argv[1] && argv[1][0]) {
            params.detectFileName = argv[1];
        } else if (strcmp(argv[0], outputArg) == 0 && argv[1] && argv[1][0]) {
            params.outputFileName = argv[1];
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

