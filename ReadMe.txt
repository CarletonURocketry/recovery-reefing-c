How to set up Raspberry Pi Pico W using Arduino IDE:
Mahin, Niraj (21/10/25)
1.	Install Arduino IDE (www.arduino.cc/en/Main/Software)
2.	In the IDE, go to File -> Preferences -> enter this URL (https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json) in ‘Additional Board Manager URLs’. This should be at the bottom of the Settings box. Click OK.
Note: If you use a different RP version, then you will need to use a different URL, this one is strictly for RP Pico series.
3.	Open boards manager from sidebar (should be the 2nd one from top) and search ‘pico’. 
4.	Install the Raspberry Pi Pico/RP2040 boards.
5.	Go to Tools -> Boards -> there should be a selection of RP pico boards. Click on that, and then select Raspberry Pi Pico W.
6.	If Raspberry Pi is currently running on micropython, then you need to manually put it into bootloader mode. For that, connect the pico to your computer while holding the bootsel button at the same time. 
7.	A mass storage device window will pop up (it won’t for Linux) but its fine just close and ignore it, however, confirm that there is a storage device added.
8.	In the IDE, open the top drop down menu and click on ‘select other board and port’.
9.	For board, select Raspberry Pi Pico W and for port, click on ‘show all ports’ and select ‘UF2 Board UF2 Devices’. Click on ok.
10.	Upload the code using the right arrow button on the top left of the screen.
11.	The terminal pops up at the bottom of the IDE that shows the status of the upload.
12.	This will disconnect the pico (storage device) and run the code you uploaded. This disconnection is normal and pico does that to run the code. It then uses the computer as a power source and runs the code but is not sharing the storage device anymore.
13.	If you want to upload the code again or a different code, connect while holding the bootsel button again. Plugging it normally would just make the pico use your computer as a power source and run the existing code.

How to Set up Raspberry Pi Pico W using Vscode: (Work in progress)
Azan Huggins Goolamalle (10/01/26)
1. Install Vscode (https://code.visualstudio.com/download)
2. Install the Raspberry Pico W Windows installer (https://www.raspberrypi.com/news/raspberry-pi-pico-windows-installer/)
3. Download RPI extension in Vscode. Do this by going to your "Extensions" tab in Vscode and typing in Raspberry Pi Pico
4. For testing, create new project from example by clicking on the raspberry pico in your left dropdown menu. Specifically choose "blink.c"
5. Test by plugging in your pico in bootsel mode, then compiling and running project, do not use debug

Common Troubleshooting
- Incorrect Drivers 
  1) Download Zadig
  2) Go to options -> List all Devices
  3) Find RPI port -> install/reinstall drivers
  4) Go to the RPI icon in the leftmost dropdown menu and hit "clean cmake". this will (hopefully) change directories to where the libraries actually are on your computer
