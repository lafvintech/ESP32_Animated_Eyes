.. __arduino_ide:

Arduino IDE
=======================

Download the Arduino IDE 2.x.x
-------------------------------

#. Visit `Download Arduino IDE <https://www.arduino.cc/en/software>`_ page.

#. Download the IDE for your OS version.

   .. image:: /Tutorial/img/Install_Arduino_IDE_1.png

.. note:: Uploading code to the Arduino UNO R4 requires Arduino IDE version 2.2 
   or higher. If your version is older, please upgrade to the latest version.

Installation
--------------

Windows
^^^^^^^^

#. Double click the ``arduino-ide_xxxx.exe`` file to run the downloaded file.

#. Read the License Agreement and agree it.

   .. image:: /Tutorial/img/Install_Arduino_IDE_2.png

#. Choose installation options.

   .. image:: /Tutorial/img/Install_Arduino_IDE_3.png

#. Choose install location. It is recommended that the software be installed on a drive other than the system drive.

   .. image:: /Tutorial/img/Install_Arduino_IDE_4.png

#. Then Finish. 

   .. image:: /Tutorial/img/Install_Arduino_IDE_5.png

MacOS
^^^^^^^^

Double click on the downloaded ``arduino_ide_xxxx.dmg`` file and follow the 
instructions to copy the **Arduino IDE.app** to the **Applications** folder, you will see the Arduino IDE installed successfully after a few seconds.

.. image:: /Tutorial/img/Install_Arduino_IDE_6.png
    :width: 800

Linux
"""""""

For the tutorial on installing the Arduino IDE 2.0 on a Linux system, please 
refer `Linux-Install Arduino IDE <https://docs.arduino.cc/software/ide-v2/tutori
als/getting-started/ide-v2-downloading-and-installing#linux>`_

Open the IDE
^^^^^^^^^^^^^

#. When you first open Arduino IDE 2.0, it automatically installs the Arduino AVR Boards, built-in libraries, and other required files.

   .. image:: /Tutorial/img/Install_Arduino_IDE_7.png

#. In addition, your firewall or security center may pop up a few times asking you if you want to install some device driver. Please install all of them.

   .. image:: /Tutorial/img/Install_Arduino_IDE_8.png

#. Now your Arduino IDE is ready!

.. note::
   In the event that some installations didn't work due to network issues or other 
   reasons, you can reopen the Arduino IDE and it will finish the rest of the 
   installation. The Output window will not automatically open after all installations 
   are complete unless you click Verify or Upload.


lnstall  drivers windows 
--------------

Next, we will cover the driver installation for the ESP32 Development Board. The driver installation may have some Differences between different computer systems. So, in what follows, let's move on to driver installation on Windows systems.A computer connected by a USB cable. Before starting the Arduino software, you will install the USB driver. When you first connect the ESP32 Board to your computer, right-click the computer icon -- >properties -- >click Device Manager under other devices or USB-Serial ,you should see an"Unknown device" icon with a small yellow warning triangle. This is the device where we need to install the driver

lnstall cp2101 drivers windows (For ESP32_DEVKIT_V1_Board)
^^^^^^^^^^^^^

For more details `refer to Install CP2101 drivers Windows <https://www.dropbox.com/scl/fo/we0z5mvl1ipsvoc814khv/AINxY8ikwhmhAU0YnsxJPtw?rlkey=virg6dinykqjwowunrqf6r1a9&st=tfkh1us0&dl=0>`_ page.

lnstall CH343 drivers windows (For ESP32_S3_Board)
^^^^^^^^^^^^^

.. role:: red
   :class: red

:red:`Check whether CH343 has been installed ESP32-S3 uses CH343 to download codes. So before using it, we need to install CH343 driver in our computers.`


1.Connect your computer and ESP32-S3 with a USB cable

   .. image:: /Tutorial/img/S3供电.jpg

1. Turn to the main interface of your computer, select “This PC” and right-click 
to select “Manage”

   .. image:: /Tutorial/img/打开设备管理.png

1. Click “Device Manager”. If your computer has installed CH343, you can see “USB -Enhances-SERIAL CH343 (COMx)”. And you can click here to move 
to the next step

   .. image:: /Tutorial/img/设备管理.png

:red:`Installing CH343`

First, download CH343 driver, click HERE to download the appropriate one based 
on your operating system.

   .. image:: /Tutorial/img/CH343下载页面.png


If you would not like to download the installation package, you can open “Breakout 
Board for ESP32/ESP32S3/Install CH343 drivers Windows”, we have prepared the 
installation package.   

   .. image:: /Tutorial/img/CH343不同系统安装包.png

Open the folder “Breakout Board for ESP32/ESP32S3/Install CH343 drivers Windows/
Windows/”

   .. image:: /Tutorial/img/CH343安装程序.png

Double click “CH343SER.EXE”.

   .. image:: /Tutorial/img/CH343安装步骤1.png

Click “INSTALL” and wait for the installation to complete.

   .. image:: /Tutorial/img/CH343安装步骤2.png

Install successfully. Close all interfaces.

   .. image:: /Tutorial/img/CH343安装步骤3.png

When ESP32-S3 is connected to computer, select “This PC”, right-click to select
“Manage” and click “Device Manager ” in the newly pop-up dialog box, and you
can see the following interface.

   .. image:: /Tutorial/img/CH343查看安装情况.png

So far, CH343 has been installed successfully. Close all dialog boxes.

.. raw:: html

   <style>
   .red {
     color: red;
     font-weight: bold;
   }
   .longtable td {
     padding: 8px;
   }
   </style>

Installing ESP32 Add-on in Arduino IDE
--------------

   .. image:: /Tutorial/img/核心包.png

①Search for esp32 and select 3.2.0 click the INSTALL button for esp32 by 
Espressif Systems.

   .. image:: /Tutorial/img/搜索ESP_3_2_0.png

②Installing,this will take a while

   .. image:: /Tutorial/img/ESP32核心包安装中.png

③Successfully installed platform esp32:3 .2.0

   .. image:: /Tutorial/img/ESP32核心包安装成功.png
