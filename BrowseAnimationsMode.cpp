/*
* Animated GIF and directory browser for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
*
* Written by: Jason Coon
* Copyright (c) 2014 Jason Coon
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

#include "BrowseAnimationsMode.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

#define ANIMATION_DISPLAY_DURATION_SECONDS 10

// Defined in GIFParseFunctions.cpp
extern unsigned long processGIFFile(const char *pathname, unsigned long(*checkForInput)());

// Defined in LightAppliance.ino
extern unsigned long checkForInput();

void BrowseAnimationsMode::run(SmartMatrix matrix, IRrecv irReceiver, SdFat sd) {
    BrowseAnimationsMode::matrix = &matrix;
    BrowseAnimationsMode::irReceiver = &irReceiver;
    BrowseAnimationsMode::sd = &sd;

    browseDirectory("/");
}

void BrowseAnimationsMode::browseDirectory(const char *path) {
    int index = 0;

    Serial.print("browsing directory: ");
    Serial.println(path);

    int numberOfFiles = countFiles(path);

    Serial.print("number of files: ");
    Serial.println(numberOfFiles);

    while (true) {
        // Clear screen
        matrix->fillScreen(COLOR_BLACK);

        // Fonts are font3x5, font5x7, font6x10, font8x13
        matrix->setFont(font5x7);

        // Static Mode Selection Text
        matrix->drawString(2, 0, COLOR_BLUE, "Select");

        matrix->setFont(font3x5);
        matrix->drawString(3, 7, COLOR_BLUE, "Pattern");
        matrix->drawString(3, 14, COLOR_BLUE, "< use >");
        matrix->swapBuffers();

        // Setup for scrolling mode
        matrix->setScrollMode(wrapForward);
        matrix->setScrollSpeed(36); // 10
        matrix->setScrollFont(font5x7);
        matrix->setScrollColor(COLOR_GREEN);
        matrix->setScrollOffsetFromEdge(22);
        matrix->scrollText("", 1);

        boolean fileSelected = false;

        while (!fileSelected) {
            // Get current directory or file name
            Serial.print("getting file name by index: ");
            Serial.println(index);

            char name[13];
            getNameByIndex(path, index, name, numberOfFiles);

            Serial.print("name: ");
            Serial.println(name);

            if (strcmp(name, "SYSTEM~1") == 0 || (name[0] == '_') || name[0] == '~') {
                index++;
                break;
            }

            char selectedPath[255];
            strcpy(selectedPath, path);
            strcat(selectedPath, name);
            strcat(selectedPath, "/");

            char* longName = getDirectoryLongName(selectedPath);
            if (strcmp(longName, "") == 0) {
                // Set name selection text
                matrix->scrollText(name, 32000);
            }
            else {
                // Set name selection text
                matrix->scrollText(longName, 32000);
            }

            unsigned long irCode = waitForIRCode();
            switch (irCode) {
                case IRCODE_HOME:
                    return;

                case IRCODE_LEFT:
                    index--;
                    if (index < 0) {
                        index = numberOfFiles - 1;
                    }
                    break;

                case IRCODE_RIGHT:
                    index++;
                    if (index >= numberOfFiles) {
                        index = 0;
                    }
                    break;

                case IRCODE_SEL:
                    // Turn off any text scrolling
                    matrix->scrollText("", 1);
                    matrix->setScrollMode(off);

                    // Clear screen
                    matrix->fillScreen(COLOR_BLACK);
                    matrix->swapBuffers();

                    // open the selected directory, or display the selected animation
                    Serial.print("selected file: ");
                    Serial.println(name);

                    Serial.print("opening selected path: ");
                    Serial.println(selectedPath);

                    SdFile file;

                    if (file.open(selectedPath)) {
                        if (file.isDir()) {
                            browseDirectory(selectedPath);
                        }
                        else {
                            runAnimation(path, index, numberOfFiles);
                        }
                        file.close();
                    }
                    fileSelected = true;
                    break;
            }
        }
    }
}

char longNameChar[100];
    
char * BrowseAnimationsMode::getDirectoryLongName(const char *directoryName) {
    char pathname[255];
    strcpy(pathname, directoryName);
    strcat(pathname, "index.txt");

    String longName;

    SdFile indexFile;
    if (indexFile.open(pathname) && indexFile.isOpen() && indexFile.isFile()) {
        int16_t c;
        while ((c = indexFile.read()) > 0) {
            char character = (char) c;
            if (character == '\r' || character == 0) {
                break;
            }

            longName.concat(character);
        }
    }
    indexFile.close();

    longName.toCharArray(longNameChar, 100);
    return longNameChar;
}

void BrowseAnimationsMode::runAnimation(const char* directoryName, int index, int numberOfFiles) {
    Serial.print("running animation file: ");
    Serial.println(directoryName);

    bool timeoutDisabled = true;

    while (true) {
        if (index < 0) {
            index = numberOfFiles - 1;
        }
        else if (index >= numberOfFiles) {
            index = 0;
        }

        // Clear screen for new animation
        matrix->fillScreen(COLOR_BLACK);
        matrix->swapBuffers();

        char name[13];
        getNameByIndex(directoryName, index, name, numberOfFiles);

        char pathname[255];
        strcpy(pathname, directoryName);
        strcat(pathname, name);

        // Calculate time in the future to terminate animation
        unsigned long timeOut = millis() + (ANIMATION_DISPLAY_DURATION_SECONDS * 1000);

        while (timeoutDisabled || timeOut > millis()) {
            unsigned long result = processGIFFile(pathname, checkForInput);

            // handle user input
            if (result == IRCODE_HOME) {
                return;
            }
            else if (result == IRCODE_LEFT) {
                index -= 2;
                break;
            }
            else if (result == IRCODE_RIGHT) {
                break;
            }
            else if (result == IRCODE_SEL) {
                // toggle timeout on/off
                timeoutDisabled = !timeoutDisabled;
            }
            else if (result != 0) {
                break;
            }
        }

        index++;
    }
}

// count the number of directories and files
int BrowseAnimationsMode::countFiles(const char* directoryName) {
    int number = 0;

    // Set the current working directory
    if (!sd->chdir(directoryName, true)) {
        return 0;
    }

    sd->vwd()->rewind();

    SdFile file;
    char fn[13];
    while (file.openNext(sd->vwd(), O_READ)) {
        file.getName(fn, 13);
        // If filename not deleted, count it
        if ((fn[0] != '_') && (fn[0] != '~')) {
            number++;
        }

        file.close();
    }

    // Set the current working directory
    sd->chdir("/", true);

    return number;
}

// Get the full path/filename of the GIF file with specified index
void BrowseAnimationsMode::getNameByIndex(const char *directoryName, int index, char *nameBuffer, int numberOfFiles) {
    Serial.println("getNameByIndex");
    char filename[13];

    // Make sure index is in range
    Serial.println("Make sure index is in range");
    if ((index >= 0) && (index < numberOfFiles)) {
        Serial.println("index is in range");

        // Set the current working directory
        Serial.println("Set the current working directory");
        if (!sd->chdir(directoryName, true)) {
            Serial.println("Could not change to directory");
            sd->errorHalt("Could not change to directory");
        }

        // Make sure file is closed before starting
        SdFile file;

        // Rewind the directory to the beginning
        Serial.println("Rewind the directory to the beginning");
        sd->vwd()->rewind();

        Serial.println("opening file");
        while ((file.openNext(sd->vwd(), O_READ)) && (index >= 0)) {
            Serial.println("getting filename");
            file.getName(filename, 13);

            Serial.print("filename: ");
            Serial.println(filename);

            // If filename is not marked as deleted, count it
            if ((filename[0] != '_') && (filename[0] != '~')) {
                Serial.println("file is not marked as deleted");
                index--;
            }
            Serial.println("closing file");
            file.close();
        }

        // Set the current working directory back to root
        Serial.println("Setting the current working directory back to root");
        if (!sd->chdir("/", true)) {
            Serial.println("Could not change to root directory");
            sd->errorHalt("Could not change to root directory");
        }

        // Copy the filename to the name buffer
        Serial.println("Copying the filename to the name buffer");
        strcpy(nameBuffer, filename);
        Serial.println("Copied the filename to the name buffer");
        Serial.print("name buffer:");
        Serial.println(nameBuffer);
    }
}

//// Check for input
//// This can be called by display 
//unsigned long BrowseAnimationsMode::checkForInput() {
//
//    boolean timeOutCondition = timeOutEnabled && (millis() > timeOut);
//    if (timeOutCondition)
//        return 0;
//
//    return readIRCode();
//}

unsigned long BrowseAnimationsMode::waitForIRCode() {

    unsigned long irCode = readIRCode();
    while ((irCode == 0) || (irCode == 0xFFFFFFFF)) {
        delay(200);
        irCode = readIRCode();
    }
    return irCode;
}

// Read an IR code
// Function will return 0 if no IR code available
unsigned long BrowseAnimationsMode::readIRCode() {

    // Is there an IR code to read ?
    unsigned long code = _readIRCode();
    if (code == 0) {
        // No code so return 0
        return 0;
    }
    // Keep reading until code changes
    while (_readIRCode() == code) {
        ;
    }
    return code;
}

// Low level IR code reading function
// Function will return 0 if no IR code available
unsigned long BrowseAnimationsMode::_readIRCode() {

    decode_results results;

    results.value = 0;

    // Attempt to read an IR code ?
    if (irReceiver->decode(&results)) {

        delay(20);

        // Prepare to receive the next IR code
        irReceiver->resume();
    }
    return results.value;
}
