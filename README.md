# C++ Lambda Web Adapter Project

This repository provides a C++ implementation of a custom AWS Lambda runtime using the Web Adapter pattern. It allows you to run a C++ test server within the Lambda environment by bridging standard HTTP traffic to your application.

## Project Structure

The project consists of two primary components:
* **`lambda_server`**: A lightweight C++ test server that includes dynamic loading capabilities for MySQL.
* **`lambda-adapter`**: The adapter responsible for handling the AWS Lambda Runtime Interface and proxying requests to the test server.
* **`bootstrap`**: The mandatory entry point script for AWS Lambda custom runtimes.

## Prerequisites

To build this project, you need the following tools and libraries:
* **Compiler**: `g++` with C++20 support (`--std=c++2a`).
* **Libraries**: 
    * `pthread`: For multi-threading support.
    * `dl`: For dynamic library loading (used by the MySQL module).
    * `libmysqlclient.so`: Required for database connectivity.
* **Cross-Compilation**: If you are building on an ARM-based machine (e.g., Apple Silicon) but targeting an x86_64 Lambda environment, the script automatically attempts to use `x86_64-linux-gnu-gcc/g++`.

## How to Build

The provided `build_lambda_image.sh` script automates the compilation and packaging process.

1. **Grant execution permissions**:
   ```bash
   chmod +x build_lambda_image.sh

2. **Run the build script**:

   ```bash
   ./build_lambda_image.sh
   
### Build Process Details:

Architecture Detection: Automatically detects if the host is aarch64 and uses the x86_64-linux-gnu toolchain for cross-compilation.

Compilation: Compiles both lambda_server and lambda-adapter with necessary include paths (src/main, src/headeronly, src/mysql_8_4) and RPATH settings.

Packaging: Creates a clean image/ directory, gathers all binaries, the bootstrap script, and the libmysqlclient.so library, then compresses them into build_image.zip.

## Deployment Package Content
The resulting build_image.zip includes:

lambda_server: The core test server logic.

lambda-adapter: The bridge between Lambda and the server.

bootstrap: The execution entry point.

libmysqlclient.so: Shared library for MySQL operations.

images.jpg: Static asset for testing purposes.

## AWS Lambda Configuration
When deploying the generated zip file to AWS, use the following settings:

Runtime: provided.al2023 or provided.al2 (Custom Runtime).

Architecture: x86_64.

Handler: Any string (the bootstrap file handles the actual execution).

## License
BSD license

