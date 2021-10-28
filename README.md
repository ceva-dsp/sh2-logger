# SH2 Logger for SH2 devices

The SH2 Logger is a command line utility that enables logging sensor data from a sensor hub over USB (FTDI UART interface).

A group of sensors will be enabled based on the operating mode and rate specified. 
The output SH2 sensor reports will be saved to a log file in DSF format.

## Requirements

* SensorHub (BNO080, FSP200, etc.) with the FTDI Adapter 
* CMAKE 
* Visual Studio (for Windows) 

## Setup

* Clone this repository using the --recursive flag with git:
```
git clone --recursive http://github.com/hcrest/sh2-logger
```

* Adjust the receive buffer latency timer. Reduce the latency timer from the default value of 16 milliseconds to 1 millisecond. 
  * For Windows application, the latency timer has been adjusted automatically. 
  * For Linux, use the following examples, assuming the device is connected to the ttyUSB0 serial port.
```
# cat /sys/bus/usb-serial/devices/ttyUSB0/latency_timer
16
# echo 1 > /sys/bus/usb-serial/devices/ttyUSB0/latency_timer
# cat /sys/bus/usb-serial/devices/ttyUSB0/latency_timer
1
```
  * For Linux, you can add a .rules file under `/etc/udev/rules.d` with
    the following: 
```
ACTION=="add", SUBSYSTEM=="usb-serial", DRIVER=="ftdi_sio", ATTR{latency_timer}="1"
```

## Building from Source

Run CMAKE to generate the makefile. CMAKE detects the platform which is run on (Windows/Linux) to generate the correct makefile.
```
cmake CMakeLists.txt
```

For Windows, open the generated solution (`sh2_logger.sln`) in the Visual Studio to build the application.

For Linux, run MAKE to compile and build the application.
```
make -f Makefile
```

## Running the application
```
Usage: sh2_logger.exe <*.json> [--template]
   *.json     - Configuration file in json format
   --template - Generate a configuration template file, 'logger.json'
```

### Generate Template Json File
Use the '--template' option to generate a batch json file template. The output file name is called "logger.json".
```
sh2_logger.exe --template
```

Next, modify the generated json file. To enable sensors, specify the operating rate of each active sensor in the json file. 
For instance, to enable the GameRV sensors at 100Hz and the Accelerometer at 200Hz. Update the "sensorList" section of the json file as below.
```
{
    "calEnable": "0x00",
    "clearDcd": false,
    "dcdAutoSave": false,
    "deviceNumber": 0,
    "orientation": "ned",
    "outDsfFile": "out.dsf",
    "sensorList": {
        "ARVR Stabilized GameRotation Vector": 0,
        "ARVR Stabilized Rotation Vector": 0,
        "Accelerometer": 200,
        "Ambient Light": 0,
        "Circle Detector": 0,
        "Flip Detector": 0,
        "Game Rotation Vector": 100,
        "Geomagnetic Rotation Vector": 0,
        "Gravity": 0,
		
		....
```

### Run The Application
To Run the DSF logger with the updated batch file
```
sh2_logger.exe logger.json
```

## Apache License, Version 2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License and
any applicable agreements you may have with CEVA, Inc.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
