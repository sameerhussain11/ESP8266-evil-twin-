ESP8266 (evil-twin) - I have changed the login portal page 

Welcome to the project. This project is inspired by https://github.com/vk496/linset and https://github.com/SpacehuhnTech/esp8266_deauther

Use this tool only on networks that you have permission for.

Note: Removed AsyncWebServer due to performance and stability Note: Reworked deauthing - now it changes channel so deauthing can work properly

Note: I don't know anything about licenes, copyrights, etc.

Credits to: Deauthing: https://github.com/SpacehuhnTech/esp8266_deauther // Learned from
ESP8266 Core: https://github.com/espressif/arduino-esp32 // Edited and used
How to use:
Connect to the AP named M1z23R with password deauther from your phone/PC.
Select the target you want (list of available APs refreshes every 30secs - page reload is required)
Click the Start Evil-Twin button and reconnect to the newly created AP named same as your target (will be open)
After connecting, make sure you chooes "Use this network as is" (may differ on different devices)
Go to your favorite browser and navigate to 192.168.4.1/admin
Once there DO NOT change your target, only start/stop deauthing and wait for someone to try and use the correct password.
Once correct password is found, AP will be restarted with default ssid M1z23R / deauther and at the bottom of a table you should be able to see something like "Successfully got password for - SSID - Password
If you have any questions, feel free to post in the issues section
