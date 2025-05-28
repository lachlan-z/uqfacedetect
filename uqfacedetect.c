#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/objdetect/objdetect_c.h>

// OpenCV parameters
const float haarScaleFactor = 1.1;
const int haarMinNeighbours = 4;
const int haarFlags = 0;
const int haarMinSize = 0;
const int haarMaxSize = 1000;
const int ellipseStartAngle = 0;
const int ellipseEndAngle = 360;
const int lineThickness = 4;
const int lineType = 8;
const int shift = 0;
const int bgraChannels = 4;
const int alphaIndex = 3;

// File locations
const char* const faceCascadeFilename = "/local/courses/csse2310/resources/a4/haarcascade_frontalface_alt2.xml";
const char* const eyesCascadeFilename = "/local/courses/csse2310/resources/a4/haarcascade_eye_tree_eyeglasses.xml";

// Error messages
const char* const usageErrorMsg
        = "Usage: ./uqfacedetect connectionlimit maxsize [port]\n";
const char* const imageFileErrorMsg = "uqfacedetect: cannot open image file for writing\n";

// Communication protocol constants
const uint32_t protocolPrefix = 0x23107231;
const uint8_t protocolDetection = 0;
const uint8_t protocolReplacement = 1;
const uint8_t protocolOutput = 2;
const uint8_t protocolError = 3;

// Error status
typedef enum { 
    EXIT_USAGE = 11,
    EXIT_IMAGE_FILE = 10
} ErrorStatus;

typedef struct {
    int connectionLimit;
    uint32_t maxSize;
    char* port;
} CmdLineParams;

void print_cmd(CmdLineParams* params)
{
    printf("CmdLineParams:\n");
    printf("    connectionLimit: %d\n", params->connectionLimit);
    printf("    maxSize: %u\n", params->maxSize);
    printf("    port: %s\n", params->port);
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

    if (argc < 2 || argc > 3) {
        usage_error();
    }

    check_empty_value(argv[0]);
    int connections = atoi(argv[0]);
    if (connections < 0 || connections > 10000) {
        usage_error();
    }
    params.connectionLimit = connections;

    check_empty_value(argv[1]);
    char* stopString;
    unsigned long maxSizeVal = strtoul(argv[1], &stopString, 10);
    if (*stopString != '\0' || maxSizeVal > UINT32_MAX) {
        usage_error();
    }

    if (maxSizeVal == 0) {
        params.maxSize = UINT32_MAX;
    } else {
        params.maxSize = (uint32_t)maxSizeVal;
    }

    if (argc == 3) {
        check_empty_value(argv[2]);
        params.port = argv[2];
    } else {
        params.port = "0";
    }
    return params;
}

void temp_img_file_check() {
    FILE* file = fopen("/tmp/imagefile.jpg", "wb");

    if (!file) {
        fprintf(stderr, imageFileErrorMsg);
        exit(EXIT_IMAGE_FILE);
    }
}

int main(int argc, char* argv[])
{
    CmdLineParams params = parse_command_line(argc, argv);
    print_cmd(&params);
    temp_img_file_check();
}
