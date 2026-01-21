# UQ Face Detect

A client-server face detection and replacement system implemented in C using OpenCV's Haar Cascade classifiers. This project provides a multithreaded server that can detect faces in images and replace them with other images, along with a client application for communicating with the server.

## Project Overview

This system consists of two main components:

1. **uqfacedetect** - A multithreaded TCP server that processes face detection and replacement requests
2. **uqfaceclient** - A client application that sends images to the server for processing

The server uses OpenCV's Haar Cascade classifiers to detect frontal faces and eyes in images. It supports concurrent client connections using POSIX threads and implements a custom binary communication protocol.

## Components

### uqfacedetect (Server)

A multithreaded server that listens for client connections and processes face detection/replacement requests.

**Features:**
- Multi-threaded architecture using pthreads for concurrent client handling
- Face detection using OpenCV Haar Cascade classifiers
- Support for face replacement operations
- Custom binary communication protocol
- Connection limiting capabilities
- Configurable maximum image size
- Automatic port allocation (or specified port)

**Usage:**
```bash
./uqfacedetect connectionlimit maxsize [port]
```

**Arguments:**
- `connectionlimit` - Maximum number of simultaneous connections (0-10000)
- `maxsize` - Maximum image size in bytes (0 for unlimited)
- `port` - Optional TCP port number (defaults to automatic allocation if not specified)

**Exit Codes:**
- 9: Unable to listen on specified port
- 10: Cannot open temporary image file for writing
- 11: Usage error (invalid command line arguments)
- 17: Unable to load cascade classifier files

### uqfaceclient (Client)

A client application for sending images to the uqfacedetect server for processing.

**Features:**
- Support for face detection operations
- Support for face replacement operations
- Read input from files or stdin
- Write output to files or stdout
- Binary protocol communication with server

**Usage:**
```bash
./uqfaceclient port [--replacefilename filename] [--detectimage filename] [--output filename]
```

**Arguments:**
- `port` - Server port number to connect to
- `--replacefilename filename` - Optional: image file to use for face replacement
- `--detectimage filename` - Optional: image file to detect faces in (defaults to stdin)
- `--output filename` - Optional: output file for processed image (defaults to stdout)

**Exit Codes:**
- 9: Cannot connect to server on specified port
- 11: Cannot open output file for writing
- 12: Received error message from server
- 13: Communication error occurred
- 14: Cannot open input file for reading
- 19: Usage error (invalid command line arguments)

## Communication Protocol

The client and server communicate using a custom binary protocol. Each message consists of:

| Bytes | Type | Description |
|-------|------|-------------|
| 4 | uint32_t | Prefix: 0x23107231 (identifies protocol messages) |
| 1 | uint8_t | Operation type (0=detect, 1=replace, 2=output, 3=error) |
| 4 | uint32_t | Image 1 size in bytes |
| M | bytes | Image 1 data |
| 4 | uint32_t | Image 2 size (only for replacement operations) |
| N | bytes | Image 2 data (only for replacement operations) |

All multi-byte integers are transmitted in little-endian format.

**Operation Types:**
- **0 (Detection)**: Detect faces in an image
- **1 (Replacement)**: Detect faces and replace them with another image
- **2 (Output)**: Server response containing processed image
- **3 (Error)**: Server error message

## Building

The project includes a Makefile for easy compilation:

```bash
make           # Build both client and server
make uqfaceclient   # Build only the client
make uqfacedetect   # Build only the server
```

**Dependencies:**
- GCC compiler with C99 support
- OpenCV libraries (core, imgcodecs, objdetect, imgproc)
- POSIX threads (pthread)

**Compiler Flags:**
- `-Wall -Wextra -pedantic` - Enable comprehensive warnings
- `-std=gnu99` - Use GNU C99 standard
- `-g` - Include debugging symbols

## Dependencies

### System Libraries
- OpenCV 2.x (C API)
  - opencv_core
  - opencv_imgcodecs
  - opencv_objdetect
  - opencv_imgproc
- pthread (POSIX threads)
- Standard C libraries (stdio, stdlib, string, unistd, netdb)

### Cascade Classifier Files
The server requires access to Haar Cascade XML files:
- Face detection: `/local/courses/csse2310/resources/a4/haarcascade_frontalface_alt2.xml`
- Eye detection: `/local/courses/csse2310/resources/a4/haarcascade_eye_tree_eyeglasses.xml`
- Response file: `/local/courses/csse2310/resources/a4/responsefile`

## Technical Implementation

### Server Architecture
- **Multi-threaded**: Each client connection is handled in a separate thread
- **Non-blocking**: Server can accept multiple simultaneous connections
- **Protocol validation**: Validates the 4-byte prefix before processing requests
- **Error handling**: Sends response file content for invalid protocol prefixes
- **Resource management**: Temporary image file handling and cascade classifier loading

### Image Processing
The server uses OpenCV's Haar Cascade classifiers with the following parameters:
- Scale factor: 1.1
- Minimum neighbors: 4
- Minimum size: 0 pixels
- Maximum size: 1000 pixels

### Client Architecture
- **Binary protocol**: Implements the custom image processing protocol
- **Flexible I/O**: Supports both file and stream-based input/output
- **Error reporting**: Comprehensive error handling with specific exit codes
- **Little-endian**: Proper handling of multi-byte integers in protocol

## Development Notes

The `toolHistory.txt` file contains development history and notes, including:
- AI assistance acknowledgment (ChatGPT) for specific implementation details
- Protocol implementation evolution
- Test case requirements and implementations

## Error Handling

Both programs include comprehensive error handling:
- Command-line argument validation
- File I/O error checking
- Network connection error handling
- Protocol validation
- Resource allocation verification

## Limitations

- Currently, the server implementation handles protocol prefix validation
- Face detection uses pre-trained Haar Cascade classifiers
- Requires specific file paths for cascade classifiers
- Image processing is synchronous within each thread

## License

This appears to be an academic project (CSSE2310 course). Please refer to your institution's academic integrity policies.

## Author

Lachlan Z
