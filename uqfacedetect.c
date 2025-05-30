#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/objdetect/objdetect_c.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_ARGS 4
#define PORT_ARG 3
#define MIN_ARGS 2
#define MIN_CONNECTIONS 0
#define MAX_CONNECTIONS 10000
#define BASE_TEN 10
#define PORT_POS 3
#define KILO 1024

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
const char* const responseFile
        = "/local/courses/csse2310/resources/a4/responsefile";

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

    if (argc < MIN_ARGS || argc > MAX_ARGS) {
        usage_error();
    }

    check_empty_value(argv[0]);
    int connections = atoi(argv[0]);
    if (connections < MIN_CONNECTIONS || connections > MAX_CONNECTIONS) {
        usage_error();
    }
    params.connectionLimit = connections;

    check_empty_value(argv[1]);
    char* stopString;
    unsigned long maxSizeVal = strtoul(argv[1], &stopString, BASE_TEN);
    if (*stopString != '\0' || maxSizeVal > UINT32_MAX) {
        usage_error();
    }

    if (maxSizeVal == 0) {
        params.maxSize = UINT32_MAX;
    } else {
        params.maxSize = (uint32_t)maxSizeVal;
    }

    if (argc == PORT_ARG) {
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
    int closeCheck = fclose(file);

    if (!file || closeCheck != 0) {
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

/* client_thread()
 * −−−−−−−−−−−−−−−
 * Handles communication with a single client in a separate thread.
 *
 * arg: a pointer to client socket fd.
 *
 * REF: ChatGPT used for prefix checking logic and send the contents of
 * responsefile
 */
void* client_thread(void* arg)
{
    int fd = *(int*)arg;
    free(arg); // malloc used before pthread_create

    uint8_t prefixBytes[sizeof(uint32_t)];
    size_t received = 0;
    ssize_t r;

    while (received < sizeof(uint32_t)) {
        r = read(fd, prefixBytes + received, sizeof(uint32_t) - received);
        if (r <= 0) {
            close(fd);
            return NULL;
        }
        received += r;
    }

    // Convert bytes to uint32_t little-endian
    uint32_t prefix = 0;
    for (int i = 0; i < (int)sizeof(uint32_t); i++) {
        prefix |= ((uint32_t)prefixBytes[i]) << ((sizeof(uint32_t) * 2) * i);
    }

    if (prefix != protocolPrefix) {
        FILE* f = fopen(responseFile ? responseFile : "responsefile", "rb");
        if (f) {
            char buf[KILO];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
                write(fd, buf, n);
            }
            fclose(f);
        }
        close(fd);
        return NULL;
    }

    close(fd);
    return NULL;
}

/* process_connections()
 * −−−−−−−−−−−−−−−
 * Accepts incoming client connections on the given server socket and spawns a
 * new thread to handle each connection using client_thread().
 *
 * fdServer: the socket file descriptor returned by start_server().
 */
void process_connections(int fdServer)
{
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;

    // Repeatedly accept connections and process data (capitalise)
    while (1) {
        fromAddrSize = sizeof(struct sockaddr_in);

        // Block, waiting for a new connection (fromAddr will be populated with
        // client address)
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        if (fd < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        // Turn our client address into a hostname and print address, hostname
        // and port
        // char hostname[NI_MAXHOST];
        // int error = getnameinfo((struct sockaddr*)&fromAddr, fromAddrSize,
        //         hostname, NI_MAXHOST, NULL, 0, 0);

        // Create a thread to deal with client
        int* data = malloc(sizeof(int));
        *data = fd;
        pthread_t threadID;
        pthread_create(&threadID, NULL, client_thread, data);
        pthread_detach(threadID);
    }
}

int main(int argc, char* argv[])
{
    CmdLineParams params = parse_command_line(argc, argv);
    // print_cmd(&params);
    temp_img_file_check();
    check_cascade_loads();
    int serverFd = start_server(params.port);
    process_connections(serverFd);
    // accept_clients(serverFd);
}
