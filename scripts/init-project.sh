#!/bin/bash

# HESIA Project Initializer
# Creates a new C project using hesia-core

PROJECT_NAME=$1
TARGET_DIR=$2

if [ -z "$PROJECT_NAME" ] || [ -z "$TARGET_DIR" ]; then
    echo "Usage: $0 <project_name> <target_dir>"
    exit 1
fi

mkdir -p "$TARGET_DIR"
cd "$TARGET_DIR"

mkdir src include

# Create CMakeLists.txt
cat > CMakeLists.txt <<EOF
cmake_minimum_required(VERSION 3.16)
project($PROJECT_NAME VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)

find_package(hesia_core REQUIRED)

add_executable($PROJECT_NAME src/main.c)
target_link_libraries($PROJECT_NAME PRIVATE hesia_core)
EOF

# Create main.c
cat > src/main.c <<EOF
#include <stdio.h>
#include <hesia/core.h>

int main() {
    hesia_error_t err;
    uint8_t random_bytes[32];
    
    printf("Initializing %s with HESIA CoreLib %s\n", "$PROJECT_NAME", hesia_version());
    
    if (hesia_random(random_bytes, sizeof(random_bytes), &err) == HESIA_STATUS_OK) {
        printf("Successfully generated 32 random bytes.\n");
    } else {
        printf("Failed to generate random bytes: %s\n", hesia_error_message(&err));
    }
    
    return 0;
}
EOF

echo "Project $PROJECT_NAME initialized in $TARGET_DIR"
echo "To build:"
echo "  cmake -S . -B build"
echo "  cmake --build build"
