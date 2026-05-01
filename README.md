# Custom Image Compressor

A high-performance C++ image compression utility that implements custom compression algorithms to reduce PNG file sizes. This project demonstrates advanced digital image processing techniques.

## Overview

The Custom Image Compressor encodes PNG images into a proprietary `.ay` binary format using a combination of **Run-Length Encoding (RLE)** and **Huffman Coding**. This approach achieves significant compression ratios by exploiting both spatial redundancy (consecutive identical pixels) and statistical frequency patterns in image data.

## Features

- **PNG I/O**: Seamless reading and writing of standard PNG images using libpng
- **Run-Length Encoding (RLE)**: Efficiently compresses consecutive identical pixels
- **Huffman Coding**: Further compresses data by assigning variable-length codes based on byte frequency
- **Custom Binary Format**: Proprietary `.ay` format with structured metadata and compressed payload
- **Bitwise Operations**: Efficient bit-level I/O for optimal storage utilization
- **Reversible Compression**: Perfect reconstruction of original images through decompression

## Technical Architecture

### Compression Pipeline

```
Input PNG
    ↓
Load Image (ColorImage class with RGBA pixels)
    ↓
Apply Run-Length Encoding (RLE)
    ↓
Analyze Byte Frequency Distribution
    ↓
Build Huffman Tree from Frequency Data
    ↓
Generate Variable-Length Huffman Codes
    ↓
Perform Bitwise Packing into .ay Format
    ↓
Output .ay File
```

### Decompression Pipeline

```
Input .ay File
    ↓
Validate Magic Number & Header
    ↓
Reconstruct Huffman Tree from Frequency Table
    ↓
Decode Huffman-Encoded Bitstream
    ↓
Decompress RLE-Encoded Data
    ↓
Reconstruct Original Image (ColorImage)
    ↓
Output PNG File
```

## File Format: .ay

### Header Structure (12 bytes, packed)

```
Bytes 0-3:    Magic Number ("OWOZ")
Bytes 4-7:    Image Width (uint32_t)
Bytes 8-11:   Image Height (uint32_t)
Bytes 12-15:  RLE Compressed Byte Count (uint32_t)
```

### Payload Structure

- **Frequency Table**: Maps each byte value (0-255) to its occurrence count in RLE data
- **Huffman-Coded Data**: Variable-length binary codes packed into 8-bit bytes

## Project Structure

```
.
├── image.h              # Image data structures (ColorImage, RGBA, GrayscaleImage)
├── main.cpp             # Main compression/decompression logic and Huffman implementation
├── rand_img.cpp         # Random image generator for testing purposes
├── data.ay              # Example compressed output file
├── Makefile             # Build configuration
├── Roadmap.txt          # Algorithm design documentation
└── README.md            # This file
```

## Building the Project

### Requirements

- **C++20** compiler (g++ or clang)
- **libpng** development library

### Compilation

```bash
# Build and run everything
make rand_img main

# Build and run the random image generator
make rand_img

# Build and run the main compressor
make main

# Clean build artifacts
```

### Build Process

The Makefile uses a pattern rule that:

1. Compiles the .cpp file with C++20 support and optimizations
2. Links against libpng
3. Automatically runs the compiled executable
4. Displays compilation status and execution output

## Usage

### Running the Compressor

```bash
make main
```

This will:

1. Compile `main.cpp` with libpng support
2. Execute the program, which:
   - Loads `input.png` from the current directory
   - Compresses it using RLE + Huffman Coding
   - Saves the result to `data.ay`
   - Decompresses `data.ay`
   - Saves the reconstructed image to `output.png`

### Testing with Random Images

```bash
make rand_img
```

This generates a random test image for validation purposes.

### Expected Output

```
  .ay Custom Image Compressor

[1/2] Loading input.png for compression...
      -> Successfully encoded to data.ay
[2/2] Loading data.ay for decompression...
      -> Successfully decoded from data.ay
```

## Algorithm Details

### Phase 1: Run-Length Encoding (RLE)

Exploits spatial redundancy by encoding consecutive identical pixels as:

```
{count, pixel_value}
```

Example:

- Input: `[Red, Red, Red, Red, Blue]`
- Output: `[4, Red, 1, Blue]`

**Limitation**: Maximum run length is 255 (1 byte). Longer sequences are split into multiple chunks.

### Phase 2: Huffman Coding

After RLE produces a byte stream, Huffman Coding assigns variable-length binary codes:

- **Frequent bytes** → Short codes (fewer bits)
- **Rare bytes** → Long codes (more bits)

This reduces overall file size when combined with RLE.

### Phase 3: Bitwise Packing

Huffman codes have variable lengths (e.g., 3 bits, 5 bits, 7 bits). Standard I/O operates on 8-bit bytes, so a bit buffer manages partial bytes:

- Collect variable-length codes into an 8-bit buffer
- When buffer is full, write to file and reset
- Handle remaining bits at end-of-file

## Performance Characteristics

- **Best Case**: Images with large uniform color regions (e.g., simple graphics, backgrounds)
- **Compression Ratio**: Typically 10-50% depending on image content
- **Trade-offs**:
  - Compression time increases with Huffman tree complexity
  - Decompression is fast (single-pass bitwise decode)
  - Memory efficient for typical image sizes