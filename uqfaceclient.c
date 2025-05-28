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
const char* const replaceFileErrorMsg
        = "uqfaceclient: cannot open the input file \"%s\" for reading\n";
const char* const outputFileErrorMsg
        = "uqfaceclient: cannot open the output file \"%s\" for writing\n";
const char* const portFailErrorMsg
        = "uqfaceclient: cannot connect to the server on port \"%s\"\n";
const char* const commErrorMsg
        = "uqfaceclient: a communication error occurred\n";
const char* const serverErrorMsg
        = "uqfaceclient: received the following error message: \"%s\"\n";

// Communication protocol structs and constants
const uint32_t protocolPrefix = 0x23107231;
const uint8_t protocolDetection = 0;
const uint8_t protocolReplacement = 1;
const uint8_t protocolOutput = 2;
const uint8_t protocolError = 3;

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

void print_cmd(CmdLineParams* params)
{
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
void usage_error()
{
    fprintf(stderr, usageErrorMsg);
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
void check_empty_value(char* value)
{
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

    if (argv[0] && strncmp(argv[0], "--", 2) != 0) {
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
FILE* determine_input_source(char* filename)
{
    FILE* stream = stdin;
    if (filename) {
        stream = fopen(filename, "rb");
        if (!stream) {
            fprintf(stderr, replaceFileErrorMsg, filename);
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
        output = fopen(params.outputFileName, "wb");
        if (!output) {
            fprintf(stderr, outputFileErrorMsg, params.outputFileName);
            exit(EXIT_OUT_FILE);
        }
    }
    return output;
}

int connect_to_server(char* port)
{
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4, for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    int err;
    if ((err = getaddrinfo("localhost", port, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, "%s\n", gai_strerror(err));
        return -1; // could not work out the address
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0); // 0 == use default protocol
    if (connect(fd, ai->ai_addr, sizeof(struct sockaddr))) {
        fprintf(stderr, portFailErrorMsg, port);
        exit(EXIT_PORT_FAIL);
    }
    return fd;
}

uint8_t* read_data(FILE* file, uint32_t* outputSize)
{
    uint32_t capacity = 1048;
    uint32_t totalRead = 0;
    uint8_t* buffer = malloc(capacity);

    while (1) {
        if (totalRead >= capacity) {
            capacity *= 2;
            uint8_t* newBuff = realloc(buffer, capacity);
            buffer = newBuff;
        }

        size_t bytesRead
                = fread(buffer + totalRead, 1, capacity - totalRead, file);
        if (bytesRead == 0) {
            break;
        }
        totalRead += (uint32_t)bytesRead;
    }
    *outputSize = totalRead;
    return buffer;
}

void send_request(int socketFd, uint8_t op, uint8_t* img1Data,
        uint32_t img1Size, uint8_t* img2Data, uint32_t img2Size)
{
    int sendFd = dup(socketFd);
    FILE* to = fdopen(sendFd, "wb");

    if (fwrite(&protocolPrefix, sizeof(uint32_t), 1, to) != 1
            || fwrite(&op, sizeof(uint8_t), 1, to) != 1
            || fwrite(&img1Size, sizeof(uint32_t), 1, to) != 1
            || fwrite(img1Data, 1, img1Size, to) != img1Size) {
        fprintf(stderr, commErrorMsg);
        exit(EXIT_COMM);
    }

    if (op == protocolReplacement) {
        if (fwrite(&img2Size, sizeof(uint32_t), 1, to) != 1
                || fwrite(img2Data, 1, img2Size, to) != img2Size) {
            fprintf(stderr, commErrorMsg);
            exit(EXIT_COMM);
        }
    }
    fflush(to);
}

void response_handler(FILE* from, FILE* output)
{
    uint32_t prefix;
    uint8_t opType;

    if (fread(&prefix, sizeof(uint32_t), 1, from) != 1
            || fread(&opType, sizeof(uint8_t), 1, from) != 1
            || prefix != protocolPrefix) {
        fprintf(stderr, commErrorMsg);
        exit(EXIT_COMM);
    }

    if (opType == protocolOutput) {
        uint32_t imageSize;
        if (fread(&imageSize, sizeof(uint32_t), 1, from) != 1) {
            fprintf(stderr, commErrorMsg);
            exit(EXIT_COMM);
        }

        uint8_t* imageData = malloc(imageSize);
        if (fread(imageData, 1, imageSize, from) != imageSize || !imageData) {
            fprintf(stderr, commErrorMsg);
            exit(EXIT_COMM);
        }

        if (fwrite(imageData, 1, imageSize, output) != imageSize) {
            fprintf(stderr, commErrorMsg);
            exit(EXIT_COMM);
        }
    } else if (opType == protocolError) {
        uint32_t errorSize;
        if (fread(&errorSize, sizeof(uint32_t), 1, from) != 1) {
            fprintf(stderr, commErrorMsg);
            exit(EXIT_COMM);
        }

        char* errorMsg = malloc(errorSize + 1);
        if (fread(errorMsg, 1, errorSize, from) != errorSize || !errorMsg) {
            fprintf(stderr, commErrorMsg);
            exit(EXIT_COMM);
        }

        errorMsg[errorSize] = '\0';
        fprintf(stderr, serverErrorMsg, errorMsg);
        exit(EXIT_SERVER);
    } else {
        fprintf(stderr, commErrorMsg);
        exit(EXIT_COMM);
    }
}

int main(int argc, char* argv[])
{
    CmdLineParams params = parse_command_line(argc, argv);

    uint8_t operation;
    FILE* detectInput = NULL;
    FILE* replaceInput = NULL;
    uint8_t* img1Data = NULL;
    uint8_t* img2Data = NULL;
    uint32_t img1Size = 0;
    uint32_t img2Size = 0;
    FILE* output = determine_output_source(params);

    if (params.replaceFileName) {
        operation = protocolReplacement;

        detectInput = determine_input_source(params.detectFileName);
        replaceInput = determine_input_source(params.replaceFileName);

        img1Data = read_data(detectInput, &img1Size);
        img2Data = read_data(replaceInput, &img2Size);
    } else {
        operation = protocolDetection;

        detectInput = determine_input_source(params.detectFileName);
        img1Data = read_data(detectInput, &img1Size);
    }
    int socketFd = connect_to_server(params.port);

    send_request(socketFd, operation, img1Data, img1Size, img2Data, img2Size);

    int receiveFd = dup(socketFd);
    FILE* from = fdopen(receiveFd, "rb");
    response_handler(from, output);
}
