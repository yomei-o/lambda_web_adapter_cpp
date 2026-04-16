C++ Lambda Web Adapter Project
This repository contains a C++ implementation of a custom AWS Lambda runtime using the Web Adapter pattern. It allows you to run a C++ test server within the Lambda environment by bridging standard HTTP traffic to your application.

Overview
The project consists of two primary components:

lambda_server: A lightweight C++ test server that includes dynamic loading capabilities for MySQL.

lambda-adapter: The adapter responsible for handling the AWS Lambda Runtime Interface and proxying requests to the test server.

Prerequisites
To build this project, you need the following:

Compiler: g++ with C++20 support (--std=c++2a).

Libraries:

pthread: For multi-threading support.

dl: For dynamic library loading (used by the MySQL module).

libmysqlclient.so: Required for database connectivity.

Cross-Compilation (Optional): If you are building on an ARM-based machine (e.g., Apple Silicon) but targeting an x86_64 Lambda environment, you must have x86_64-linux-gnu-gcc/g++ installed.

Build Instructions
The provided build_lambda_image.sh script automates the compilation and packaging process.

Grant execution permissions:

Bash
chmod +x build_lambda_image.sh
Run the build script:

Bash
./build_lambda_image.sh
What the script does:
Architecture Detection: Automatically switches to the x86_64-linux-gnu cross-compiler if the host machine is aarch64.

Compilation: Compiles lambda_server and lambda-adapter using the C++20 standard.

Workspace Cleanup: Creates a clean image/ directory.

Packaging: Collects the binaries, the bootstrap script, and necessary shared libraries (libmysqlclient.so), then zips them into build_image.zip.

Deployment Package Structure
The resulting build_image.zip contains:

lambda_server: The core test server logic.

lambda-adapter: The bridge between Lambda and the server.

bootstrap: The entry point script required by AWS Lambda Custom Runtimes.

libmysqlclient.so: The shared library for MySQL operations.

images.jpg: Static asset for testing purposes.

AWS Lambda Configuration
When deploying to AWS, use the following settings:

Runtime: provided.al2023 or provided.al2 (Custom Runtime).

Architecture: x86_64 (as defined by the build script's default target).

Handler: Any value (the bootstrap file handles the execution).
