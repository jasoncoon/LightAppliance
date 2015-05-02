#ifndef BrowseAnimationsMode_H
#define BrowseAnimationsMode_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"
#include "SdFat.h"

class BrowseAnimationsMode{

public:
    void run(SmartMatrix matrix, IRrecv irReceiver, SdFat sd);

private:
    SmartMatrix *matrix;
    IRrecv *irReceiver;
    SdFat *sd;

    void browseDirectory(const char *path);
    void runAnimation(const char* directoryName, int index, int numberOfFiles);

    char * getDirectoryLongName(const char *directoryName);

    int countFiles(const char *directoryName);

    void getNameByIndex(const char *directoryName, int index, char *nameBuffer, int numberOfFiles);

    // unsigned long checkForInput();

    unsigned long waitForIRCode();
    unsigned long readIRCode();
    unsigned long _readIRCode();
};

#endif
