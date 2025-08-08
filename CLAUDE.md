# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C99 communication library implementing the PLANET-Q TLV (Tag-Length-Value) communication protocol. The library provides encoding and decoding functions for a packet format used in embedded systems communication, featuring:

- TLV-based payload structure with variable-length data
- Packet framing with start/end markers (0x7E/0x7F)
- CRC16 checksum validation 
- Byte stuffing for data integrity
- Stream-based byte-by-byte processing suitable for embedded systems

## Build System

The project uses CMake with presets for different configurations:

### Configure and Build Commands
```bash
# Debug build (default)
cmake --preset default
cmake --build build/default

# Release build
cmake --preset release  
cmake --build build/release

# Embedded build (MinSizeRel, no tests)
cmake --preset embedded
cmake --build build/embedded
```

### Test Commands
```bash
# Run tests after building
cmake --build build/default --target test
# Or using ctest directly
ctest --preset default

# For release builds
ctest --preset release
```

### Single Test Execution
```bash
# Run the test executable directly
./build/default/tests/test_pq_com_format
```

## Architecture

### Core Components

- **lib/src/pq_com_format.c**: Main implementation containing encode/decode state machines
- **lib/include/pq_com_format/pq_com_format.h**: Public API and type definitions
- **lib/src/internal.h**: Internal definitions (currently minimal)
- **tests/test_pq_com_format.c**: Test suite

### Key Data Structures

- `pq_com_format_t`: Main packet structure containing destination_id, source_id, payload, CRC, etc.
- `decode_state_t`: Internal state machine states for byte-by-byte decoding
- Result enums for decode/encode operations with specific error codes

### Protocol Format

Packets follow this structure:
```
[0x7E] [dest_id] [src_id] [payload_len] [payload...] [crc_high] [crc_low] [0x7F]
```

With byte stuffing applied:
- 0x7E → 0x7D 0x81
- 0x7F → 0x7D 0x80  
- 0x7D → 0x7D 0x7D

### Processing Model

Both encoding and decoding use streaming, stateful processing:
- **Decoding**: Call `pq_com_format_decode()` with each received byte
- **Encoding**: Call `pq_com_format_encode()` repeatedly to get each output byte
- Static state variables maintain context between calls

## Development Notes

- Library targets C99 standard with strict compiler warnings enabled
- Supports both static and shared library builds
- Embedded configuration optimizes for size (MinSizeRel) and disables tests
- All functions use byte-level processing suitable for resource-constrained environments