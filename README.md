IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
=============================================================================

Makes the LED Matrix into a display appliance that can display:
 * Open/Closed sign for business
 * Analog and Digital clock display modes with time and temperature
 * Mood lighting effects
 * Various colorful graphic effects (including fractals and plasmas) for attention getting fun
 * Animated GIFs (32x32 resolution) in general and holiday categories
 * Games
  
Requires the SmartMatrix Library by Pixelmatix
See: https://github.com/pixelmatix/SmartMatrix/releases

Required Hardware
-----------------
*  the LED matrix. SparkFun (SF) part number: COM-12584
*  a Teensy 3.1 microcontroller. SF part number: DEV-12646
*  a IR receiver diode. SF part number: SEN-10266
*  an IR remote control. SF part number: COM-11759
*  a 5 VDC 2 amp (min) power supply. SF part number: TOL-11296 or equivalent

Optional Hardware
-----------------
*  32.768 KHz crystal for Teensy 3.1 RTC operation. SF part number: COM-00540
*  DS18B20 temperature sensor. SF part number: SEN-00245
*  Breakout board for SD-MMC memory card. SF part number: BOB-11403
*  SD memory card up to 2 GBytes in size

Light Appliance SD Memory Card Preparation
—————————————————————————————————————————-
The Light Appliance uses an SD memory card (2 GBytes or less) for storage of animated GIF files. The
SD card is prepared as follows:

1. The SD memory card must be formatted in FAT16 format. You do this using a formatting program
   such as https://www.sdcard.org/downloads/formatter_4/ or by using the file manager/explorer on
   your computer. Don't do a quick format; do a full format of the card.

2. Create the following directories off of the root directory of the SD card: 
   gengifs  – which will contain the general animated GIF files
   xmasgifs - which will contain the Christmas themed animated GIF files
   halogifs - which will contain the Halloween themed animated GIF files
   valgifs  - which will contain the Valentine's day themed animated GIF files
   4thgifs  - which will contain the 4th of July themed animated GIF files

3. Download the zip file from the https://github.com/CraigLindley/GIFS repository and unzip it.

4. Copy the animated GIF files from the unzipped file to the corresponding directories on the SD card.

NOTE: you can add your own animated GIF files to these directories as long as they are 32x32 resolution.


A somewhat dated video of the Light Appliance in operation is available on youtube.com
[![Video](http://img.youtube.com/vi/VrOEJqX1-mE/0.jpg)](http://www.youtube.com/watch?v=VrOEJqX1-mE)  


Games
-----

Breakout  
[![Breakout](http://img.youtube.com/vi/j8szcxkgTSU/0.jpg)](http://www.youtube.com/watch?v=j8szcxkgTSU)  

Snake  
[![Snake](http://img.youtube.com/vi/G5TUtR3zWg4/0.jpg)](http://www.youtube.com/watch?v=G5TUtR3zWg4)  

Pac-Man  
[![Pac-Man](http://img.youtube.com/vi/f6wRntnCA6A/0.jpg)](http://www.youtube.com/watch?v=f6wRntnCA6A)  
