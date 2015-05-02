/*
 * Animated GIFs Display Code for 32x32 RGB LED Matrix
 *
 * This file contains code to parse animated GIF files
 *
 * Written by: Craig A. Lindley
 * Version: 1.1
 * Last Update: 07/04/2014
 *
 * Copyright (c) 2014 Craig A. Lindley
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// NOTE: setting this to 1 will cause parsing to fail at the present time
#define DEBUG 0

#include "Codes.h"

#include "SmartMatrix.h"
extern SmartMatrix matrix;

#include <SdFat.h>
extern SdFat sd;
extern SdFile file;

const int WIDTH  = 32;
const int HEIGHT = 32;

// Defined in LZWFunctions.cpp
extern void lzw_decode_init (int csize, byte *buf);
extern void decompressAndDisplayFrame();
extern byte imageData[1024];
extern byte imageDataBU[1024];

// Error codes
#define ERROR_NONE		    0
#define ERROR_FILEOPEN		   -1
#define ERROR_FILENOTGIF	   -2
#define ERROR_BADGIFFORMAT         -3
#define ERROR_UNKNOWNCONTROLEXT	   -4

#define GIFHDRTAGNORM   "GIF87a"  // tag in valid GIF file
#define GIFHDRTAGNORM1  "GIF89a"  // tag in valid GIF file
#define GIFHDRSIZE 6

// Global GIF specific definitions
#define COLORTBLFLAG    0x80
#define INTERLACEFLAG   0x40
#define TRANSPARENTFLAG 0x01

#define NO_TRANSPARENT_INDEX -1

// Disposal methods
#define DISPOSAL_NONE       0
#define DISPOSAL_LEAVE      1
#define DISPOSAL_BACKGROUND 2
#define DISPOSAL_RESTORE    3

// RGB data structure
typedef struct {
    byte Red;
    byte Green;
    byte Blue;
} 
RGB;

// Logical screen descriptor attributes
int lsdWidth;
int lsdHeight;
int lsdPackedField;
int lsdAspectRatio;
int lsdBackgroundIndex;

// Table based image attributes
int tbiImageX;
int tbiImageY;
int tbiWidth;
int tbiHeight;
int tbiPackedBits;
boolean tbiInterlaced;

int frameDelay;
int transparentColorIndex;
int prevBackgroundIndex;
int prevDisposalMethod;
int disposalMethod;
int lzwCodeSize;
boolean keyFrame;
int rectX;
int rectY;
int rectWidth;
int rectHeight;

int colorCount;
RGB gifPalette[256];

byte lzwImageData[1024];
char tempBuffer[260];

// Backup the read stream by n bytes
void backUpStream(int n) {
    file.seekCur(-n);
}

// Read a file byte
int readByte() {

    int b = file.read();
    if (b == -1) {
        Serial.println("Read error or EOF occurred");
    }
    return b;
}

// Read a file word
int readWord() {

    int b0 = readByte();
    int b1 = readByte();    
    return (b1 << 8) | b0;
}

// Read the specified number of bytes into the specified buffer
int readIntoBuffer(void *buffer, int numberOfBytes) {

    int result = file.read(buffer, numberOfBytes);
    if (result == -1) {
        Serial.println("Read error or EOF occurred");
    }
    return result;
}

// Fill a portion of imageData buffer with a color index
void fillImageDataRect(byte colorIndex, int x, int y, int width, int height) {

    int yOffset;

    for (int yy = y; yy < height + y; yy++) {
        yOffset = yy * WIDTH;
        for (int xx = x; xx < width + x; xx++) {
            imageData[yOffset + xx] = colorIndex;
        }
    }
}

// Fill entire imageData buffer with a color index
void fillImageData(byte colorIndex) {

    memset(imageData, colorIndex, sizeof(imageData));
}

// Copy image data in rect from a src to a dst
void copyImageDataRect(byte *src, byte *dst, int x, int y, int width, int height) {

    int yOffset, offset;

    for (int yy = y; yy < height + y; yy++) {
        yOffset = yy * WIDTH;
        for (int xx = x; xx < width + x; xx++) {
            offset = yOffset + xx;
            dst[offset] = src[offset];
        }
    }
}

// Make sure the file is a Gif file
boolean parseGifHeader() {

    char buffer[10];

    readIntoBuffer(buffer, GIFHDRSIZE);

    if ((strncmp(buffer, GIFHDRTAGNORM,  GIFHDRSIZE) != 0) &&
        (strncmp(buffer, GIFHDRTAGNORM1, GIFHDRSIZE) != 0))  {
        return false;
    }	
    else    {
        return true;
    }
}

// Parse the logical screen descriptor
void parseLogicalScreenDescriptor() {

    lsdWidth = readWord();
    lsdHeight = readWord();
    lsdPackedField = readByte();
    lsdBackgroundIndex = readByte();
    lsdAspectRatio = readByte();

#if DEBUG == 1
    Serial.print("lsdWidth: "); 
    Serial.println(lsdWidth);
    Serial.print("lsdHeight: "); 
    Serial.println(lsdHeight);
    Serial.print("lsdPackedField: "); 
    Serial.println(lsdPackedField, HEX);
    Serial.print("lsdBackgroundIndex: "); 
    Serial.println(lsdBackgroundIndex);
    Serial.print("lsdAspectRatio: "); 
    Serial.println(lsdAspectRatio);
#endif
}

// Parse the global color table
void parseGlobalColorTable() {

    // Does a global color table exist?
    if (lsdPackedField & COLORTBLFLAG) {

        // A GCT was present determine how many colors it contains
        colorCount = 1 << ((lsdPackedField & 7) + 1);

#if DEBUG == 1
        Serial.print("Global color table with ");
        Serial.print(colorCount);
        Serial.println(" colors present");
#endif
        // Read color values into the palette array
        int colorTableBytes = sizeof(RGB) * colorCount;
        readIntoBuffer(gifPalette, colorTableBytes);
    }    
}

// Parse plain text extension and dispose of it
void parsePlainTextExtension() {

#if DEBUG == 1
    Serial.println("\nProcessing Plain Text Extension");
#endif
    // Read plain text header length
    byte len = readByte();

    // Consume plain text header data
    readIntoBuffer(tempBuffer, len);

    // Consume the plain text data in blocks
    len = readByte();
    while (len != 0) {
        readIntoBuffer(tempBuffer, len);
        len = readByte();
    }
}

// Parse a graphic control extension
void parseGraphicControlExtension() {

#if DEBUG == 1
    Serial.println("\nProcessing Graphic Control Extension");
#endif
    int len = readByte();	// Check length
    if (len != 4) {
        Serial.println("Bad graphic control extension");
    }

    int packedBits = readByte();
    frameDelay = readWord();
    transparentColorIndex = readByte();

    if ((packedBits & TRANSPARENTFLAG) == 0) {
        // Indicate no transparent index
        transparentColorIndex = NO_TRANSPARENT_INDEX;
    }
    disposalMethod = (packedBits >> 2) & 7;
    if (disposalMethod > 3) {
        disposalMethod = 0;
        Serial.println("Invalid disposal value");
    }

    readByte();	// Toss block end

#if DEBUG == 1
    Serial.print("PacketBits: ");
    Serial.println(packedBits, HEX);
    Serial.print("Frame delay: ");
    Serial.println(frameDelay);
    Serial.print("transparentColorIndex: ");
    Serial.println(transparentColorIndex);
    Serial.print("disposalMethod: ");
    Serial.println(disposalMethod);
#endif
}

// Parse application extension
void parseApplicationExtension() {

    memset(tempBuffer, 0, sizeof(tempBuffer));

#if DEBUG == 1
    Serial.println("\nProcessing Application Extension");
#endif

    // Read block length
    byte len = readByte();

    // Read app data
    readIntoBuffer(tempBuffer, len);

#if DEBUG == 1
    // Conditionally display the application extension string
    if (strlen(tempBuffer) != 0) {
        Serial.print("Application Extension: ");
        Serial.println(tempBuffer);
    }	
#endif

    // Consume any additional app data
    len = readByte();
    while (len != 0) {
        readIntoBuffer(tempBuffer, len);
        len = readByte();
    }
}

// Parse comment extension
void parseCommentExtension() {

#if DEBUG == 1
    Serial.println("\nProcessing Comment Extension");
#endif

    // Read block length
    byte len = readByte();
    while (len != 0) {
        // Clear buffer
        memset(tempBuffer, 0, sizeof(tempBuffer));

        // Read len bytes into buffer
        readIntoBuffer(tempBuffer, len);		

#if DEBUG == 1
        // Display the comment extension string
        if (strlen(tempBuffer) != 0) {
            Serial.print("Comment Extension: ");
            Serial.println(tempBuffer);
        }
#endif
        // Read the new block length
        len = readByte();
    }
}

// Parse file terminator
int parseGIFFileTerminator() {

#if DEBUG == 1
    Serial.println("\nProcessing file terminator");
#endif

    byte b = readByte();	
    if (b != 0x3B) {

#if DEBUG == 1
        Serial.print("Terminator byte: ");
        Serial.println(b, HEX);
#endif
        Serial.println("Bad GIF file format - Bad terminator");
        return ERROR_BADGIFFORMAT;
    }	
    else	{
        return ERROR_NONE;
    }
}

// Parse table based image data
void parseTableBasedImage() {

#if DEBUG == 1
    Serial.println("\nProcessing Table Based Image Descriptor");
#endif

    // Parse image descriptor
    tbiImageX = readWord();
    tbiImageY = readWord();
    tbiWidth = readWord();
    tbiHeight = readWord();
    tbiPackedBits = readByte();

#if DEBUG == 1
    Serial.print("tbiImageX: ");
    Serial.println(tbiImageX);
    Serial.print("tbiImageY: ");
    Serial.println(tbiImageY);
    Serial.print("tbiWidth: ");
    Serial.println(tbiWidth);
    Serial.print("tbiHeight: ");
    Serial.println(tbiHeight);
    Serial.print("PackedBits: ");
    Serial.println(tbiPackedBits, HEX);
#endif

    // Is this image interlaced ?
    tbiInterlaced = ((tbiPackedBits & INTERLACEFLAG) != 0);

#if DEBUG == 1
    Serial.print("Image interlaced: ");
    Serial.println((tbiInterlaced != 0) ? "Yes" : "No");     
#endif

    // Does this image have a local color table ?
    boolean localColorTable =  ((tbiPackedBits & COLORTBLFLAG) != 0);

    if (localColorTable) {
        int colorBits = ((tbiPackedBits & 7) + 1);
        colorCount = 1 << colorBits;

#if DEBUG == 1
        Serial.print("Local color table with ");
        Serial.print(colorCount);
        Serial.println(" colors present");
#endif
        // Read colors into palette
        int colorTableBytes = sizeof(RGB) * colorCount;
        readIntoBuffer(gifPalette, colorTableBytes);
    }

    // One time initialization of imageData before first frame
    if (keyFrame) {
        if (transparentColorIndex == NO_TRANSPARENT_INDEX) {
            fillImageData(lsdBackgroundIndex);
        }    
        else    {
            fillImageData(transparentColorIndex);
        }
        keyFrame = false;

        rectX = 0;
        rectY = 0;
        rectWidth = WIDTH;
        rectHeight = HEIGHT;
    }
    // Don't clear matrix screen for these disposal methods
    if ((prevDisposalMethod != DISPOSAL_NONE) && (prevDisposalMethod != DISPOSAL_LEAVE)) {
        matrix.fillScreen({
            0,0,0        }
        );
    }

    // Process previous disposal method
    if (prevDisposalMethod == DISPOSAL_BACKGROUND) {
        // Fill portion of imageData with previous background color
        fillImageDataRect(prevBackgroundIndex, rectX, rectY, rectWidth, rectHeight);
    }    
    else if (prevDisposalMethod == DISPOSAL_RESTORE) {
        copyImageDataRect(imageDataBU, imageData, rectX, rectY, rectWidth, rectHeight);
    }

    // Save disposal method for this frame for next time
    prevDisposalMethod = disposalMethod;

    if (disposalMethod != DISPOSAL_NONE) {
        // Save dimensions of this frame
        rectX = tbiImageX;
        rectY = tbiImageY;
        rectWidth = tbiWidth;
        rectHeight = tbiHeight;

        if (disposalMethod == DISPOSAL_BACKGROUND) {
            if (transparentColorIndex != NO_TRANSPARENT_INDEX) {
                prevBackgroundIndex = transparentColorIndex;
            }    
            else    {
                prevBackgroundIndex = lsdBackgroundIndex;   
            }
        }    
        else if (disposalMethod == DISPOSAL_RESTORE) {
            copyImageDataRect(imageData, imageDataBU, rectX, rectY, rectWidth, rectHeight);
        }
    }

    // Read the min LZW code size
    lzwCodeSize = readByte();

#if DEBUG == 1
    Serial.print("LzwCodeSize: ");
    Serial.println(lzwCodeSize);
#endif

    // Gather the lzw image data
    // NOTE: the dataBlockSize byte is left in the data as the lzw decoder needs it
    int offset = 0;
    int dataBlockSize = readByte();
    while (dataBlockSize != 0) {
        backUpStream(1);
        dataBlockSize++;
        readIntoBuffer(lzwImageData + offset, dataBlockSize);
        offset += dataBlockSize;
        dataBlockSize = readByte();
    }
    // Process the animation frame for display

    // Initialize the LZW decoder for this frame
    lzw_decode_init(lzwCodeSize, lzwImageData);

    // Decompress LZW data and display the frame
    decompressAndDisplayFrame();

    // Make sure there is at least some delay between frames
    if (frameDelay < 6) {
        frameDelay = 6;
    }
    delay(frameDelay * 10);

    // Graphic control extension is for a single frame
    // Remove its influence
    transparentColorIndex = NO_TRANSPARENT_INDEX;
    disposalMethod = DISPOSAL_NONE;
}

// Parse gif data
unsigned long parseData(unsigned long (*checkForInput)()) {

#if DEBUG == 1
    Serial.println("\nParsing Data Block");
#endif

    boolean done = false;	
    while (! done) {

        // Determine what kind of data to process
        byte b = readByte();

        if (b == 0x2c) {
            // Parse table based image
            parseTableBasedImage();

        }	
        else if (b == 0x21) {
            // Parse extension
            b = readByte();

            // Determine which kind of extension to parse
            switch (b) {
            case 0x01:
                // Plain test extension
                parsePlainTextExtension();
                break;
            case 0xf9:
                // Graphic control extension
                parseGraphicControlExtension();
                break;
            case 0xfe:
                // Comment extension
                parseCommentExtension();
                break;
            case 0xff:
                // Application extension
                parseApplicationExtension();
                break;
            default:
                Serial.print("Unknown control extension: ");
                Serial.println(b, HEX);
                return ERROR_UNKNOWNCONTROLEXT;
            }
        }	
        else	{
            done = true;

            // Push unprocessed byte back into the stream for later processing
            backUpStream(1);
        }
        // Check to see if user wants to abort current animation
        int input = checkForInput();
        if((input == IRCODE_HOME) || (input == IRCODE_RIGHT) || (input == IRCODE_LEFT)) {
            return input;
        }
    }
    return ERROR_NONE;
}

// Attempt to parse the gif file
unsigned long processGIFFile(const char *pathname, unsigned long (*checkForInput)()) {

    // Initialize variables
    keyFrame = true;
    prevDisposalMethod = DISPOSAL_NONE;
    transparentColorIndex = NO_TRANSPARENT_INDEX;

    Serial.print("Pathname: ");
    Serial.println(pathname);

    file.close();

    // Attempt to open the file for reading
    if (! file.open(pathname)) {
        Serial.println("Error opening GIF file");
        return ERROR_FILEOPEN;
    }
    // Validate the header
    if (! parseGifHeader()) {
        Serial.println("Not a GIF file");
        file.close();
        return ERROR_FILENOTGIF;
    }
    // If we get here we have a gif file to process

    // Parse the logical screen descriptor
    parseLogicalScreenDescriptor();

    // Parse the global color table
    parseGlobalColorTable();

    // Parse gif data
    unsigned long result = parseData(checkForInput);
    if (result != ERROR_NONE) {
        Serial.println("Error: ");
        Serial.println(result);
        Serial.println(" occurred during parsing of data");
        file.close();
        return result;
    }
    // Parse the gif file terminator
    result = parseGIFFileTerminator();
    file.close();

    Serial.println("Success");
    return result;
}































