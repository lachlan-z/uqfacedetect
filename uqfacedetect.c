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
const char* const faceCascadeFilename = "/local/courses/csse2310/resources/a4/"
                                        "haarcascade_frontalface_alt2.xml";
const char* const eyesCascadeFilename = "/local/courses/csse2310/resources/a4/"
                                        "haarcascade_eye_tree_eyeglasses.xml";

// Error messages
const char* const usageErrorMsg
        = "Usage: ./uqfacedetect connectionlimit maxsize [port]\n";
const char* const imageFileErrorMsg
        = "uqfacedetect: cannot open image file for writing\n";
const char* const cascadeErrorMsg
        = "uqfacedetect: unable to load a cascade classifier\n";
const char* const portErrorMsg
        = "uqfacedetect: unable to listen on given port \"%s\"\n";

// Communication protocol constants
const uint32_t protocolPrefix = 0x23107231;
const uint8_t protocolDetection = 0;
const uint8_t protocolReplacement = 1;
const uint8_t protocolOutput = 2;
const uint8_t protocolError = 3;

// Error status
typedef enum {
    EXIT_USAGE = 11,
    EXIT_IMAGE_FILE = 10,
    EXIT_CASCADE = 17,
    EXIT_PORT = 9
} ErrorStatus;

// Struct tor store cmd line parameters
typedef struct {
    int connectionLimit;
    uint32_t maxSize;
    char* port;
} CmdLineParams;

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

/* temp_img_file_check()
 * ------------------
 * Checks if the temporary image file can be opened, truncated and closed
 * without fail.
 *
 * Errors: prints image file error msg to stderr and exits with status 10 if any
 * step fails.
 */
void temp_img_file_check()
{
    FILE* file = fopen("/tmp/imagefile.jpg", "wb");
    int close_check = fclose(file);

    if (!file || close_check != 0) {
        fprintf(stderr, imageFileErrorMsg);
        exit(EXIT_IMAGE_FILE);
    }
}

/* check_cascade_loads()
 * ------------------
 * Checks if both cascade files can be opened with cvLoad.
 *
 * Errors: If they cannot be opened print cascade exit msg to stderr and exit
 * with its status.
 */
void check_cascade_loads()
{
    CvHaarClassifierCascade* faceCascade = (CvHaarClassifierCascade*)cvLoad(
            faceCascadeFilename, NULL, NULL, NULL);
    CvHaarClassifierCascade* eyesCascade = (CvHaarClassifierCascade*)cvLoad(
            eyesCascadeFilename, NULL, NULL, NULL);

    if (!faceCascade || !eyesCascade) {
        fprintf(stderr, cascadeErrorMsg);
        exit(EXIT_CASCADE);
    }
}

/* port_error()
 * ------------------
 * Prints portErrorMsg to stderr and exits with EXIT_PORT status.
 *
 * Errors: Always errors with portErrorMsg and status.
 */
void port_error(char* port)
{
    fprintf(stderr, portErrorMsg, port);
    exit(EXIT_PORT);
}

/* start_server()
 * ------------------
 * Sets up a server socket and binds it on the specified port, then listens for
 * connections.
 *
 * port: a string of the port number to bind to
 *
 * Returns: fd for the socket
 * Errors: If any address info, binding, listening or querying fails it calls
 * port_error(port).
 */
int start_server(char* port)
{
    struct addrinfo* ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4  for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // Because we want to bind with it on all
                                 // of our interfaces (if first argument
                                 // to getaddrinfo() is NULL)
    int err;
    if ((err = getaddrinfo(NULL, port, &hints, &ai))) {
        freeaddrinfo(ai);
        port_error(port); // could not work out the address
    }

    // create a socket and bind it to a port
    int serverFd = socket(AF_INET, SOCK_STREAM, 0); // 0 == use default protocol
    if (bind(serverFd, ai->ai_addr, sizeof(struct sockaddr))) {
        port_error(port);
    }

    if (listen(serverFd, 0) != 0) {
        port_error(port);
    }

    // Which port did we get?
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(serverFd, (struct sockaddr*)&ad, &len)) {
        port_error(port);
    }

    fprintf(stderr, "%d\n", ntohs(ad.sin_port));
    fflush(stderr);

    return serverFd;
}

/* accept_clients()
 * −−−−−−−−−−−−−−−
 * Accepts incoming client connections in a loop on the given server socket fd.
 *
 * serverFd: a valid socket fd.
 */
void accept_clients(int serverFd)
{
    int connFd;
    // change 0, 0 on next line to get info about other end
    while (connFd = accept(serverFd, 0, 0), connFd >= 0) {
        FILE* stream = fdopen(connFd, "w");
        fflush(stream);
        fclose(stream);
    }
}

int main(int argc, char* argv[])
{
    CmdLineParams params = parse_command_line(argc, argv);
    //print_cmd(&params);
    temp_img_file_check();
    check_cascade_loads();
    int serverFd = start_server(params.port);
    accept_clients(serverFd);
}
