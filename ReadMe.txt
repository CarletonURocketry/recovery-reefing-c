# Setting Up Raspberry Pi Pico W with Arduino IDE

**Authors:** Mahin, Niraj  
**Date:** 21/10/25  

---

## Step-by-Step Setup Guide

### 1. Install Arduino IDE
Download and install the latest Arduino IDE from the official site:  
[https://www.arduino.cc/en/Main/Software](https://www.arduino.cc/en/Main/Software)

---

### 2. Add the Pico Board Manager URL
1. In the Arduino IDE, go to **File → Preferences**.  
2. Find the field labeled **“Additional Board Manager URLs”** (at the bottom of the Settings window).  
3. Paste this URL:
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json


4. Click **OK** to save.

**Note:**  
This URL is specifically for the **RP2040 / Raspberry Pi Pico series**.  
If you are using a different RP board, use the corresponding JSON URL for that version.

---

### 3. Install Raspberry Pi Pico/RP2040 Boards
1. Open the **Boards Manager** from the left sidebar (it’s the second icon from the top).  
2. Search for **“pico”**.  
3. Install the package named **“Raspberry Pi Pico / RP2040 Boards”** by Earle Philhower.

---

### 4. Select the Pico W Board
Go to **Tools → Board → Raspberry Pi RP2040 Boards → Raspberry Pi Pico W**.

---

### 5. Enter Bootloader Mode (if needed)
If your Pico is currently running **MicroPython**, you’ll need to manually enter bootloader mode:
1. Hold down the **BOOTSEL** button on the Pico.
2. While holding it, connect the Pico to your computer using a USB cable.
3. Release the button after connecting.

A mass storage device window will appear (on Windows/macOS).  
On Linux, this might not pop up, but the device will still be detected.

---

### 6. Select the Board and Port in Arduino IDE
1. In the IDE, open the top dropdown menu and click **Select other board and port**.  
2. For **Board**, choose **Raspberry Pi Pico W**.  
3. For **Port**, click **Show all ports** and select **UF2 Board (UF2 Devices)**.  
4. Click **OK**.

---

### 7. Upload Code
1. Click the **Upload** button (right arrow icon) at the top-left of the IDE.  
2. The terminal at the bottom will show upload progress.

---

### 8. After Upload
Once the upload is complete:
- The Pico will automatically disconnect as a storage device.
- This is **normal** — it means the code is running.
- The Pico will continue to draw power from your computer but will no longer appear as a drive.

---

### 9. Re-uploading Code
To upload a new or different sketch:
1. Hold the **BOOTSEL** button again while connecting the Pico to your computer.  
2. Plugging it in **without** holding BOOTSEL will simply power it and run the existing code.

---

