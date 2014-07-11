/*
 * Animated GIFs Display Code for 32x32 RGB LED Matrix
 *
 * This file contains code to enumerate and select animated GIF files by name
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

#include <SdFat.h>
extern SdFat sd;

SdFile file;

int numberOfFiles;

// Enumerate and possibly display the animated GIF filenames in GIFS directory
int enumerateGIFFiles(const char *directoryName, boolean displayFilenames) {

    numberOfFiles = 0;

    // Set the current working directory
    if (! sd.chdir(directoryName, true)) {
        sd.errorHalt("Could not change to gifs directory");
    }
    sd.vwd()->rewind();

    char fn[13];
    while (file.openNext(sd.vwd(), O_READ)) {
        file.getFilename(fn);
        // If filename not deleted, count it
        if (fn[0] != '_') {
            numberOfFiles++;
            if (displayFilenames) {
                Serial.println(fn);
                delay(20);
            }
        }
        file.close();
    }
    // Set the current working directory
    if (! sd.chdir("/", true)) {
        sd.errorHalt("Could not change to root directory");
    }
    return numberOfFiles;
}

// Get the full path/filename of the GIF file with specified index
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer) {

    char filename[13];

    // Make sure index is in range
    if ((index >= 0) && (index < numberOfFiles)) {

        // Set the current working directory
        if (! sd.chdir(directoryName, true)) {
            sd.errorHalt("Could not change to gifs directory");
        }

        // Make sure file is closed before starting
        file.close();

        // Rewind the directory to the beginning
        sd.vwd()->rewind();

        while ((file.openNext(sd.vwd(), O_READ)) && (index >= 0)) {

            file.getFilename(filename);

            // If filename is not marked as deleted, count it
            if ((filename[0] != '_') && (filename[0] != '~')) {
                index--;
            }
            file.close();
        }
        // Set the current working directory back to root
        if (! sd.chdir("/", true)) {
            sd.errorHalt("Could not change to root directory");
        }
        // Copy the directory name into the pathname buffer
        strcpy(pnBuffer, directoryName);

        // Append the filename to the pathname
        strcat(pnBuffer, filename);
    }
}

// Return a random animated gif path/filename from the specified directory
void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer) {

    int index = random(numberOfFiles);
    getGIFFilenameByIndex(directoryName, index, pnBuffer);
}







