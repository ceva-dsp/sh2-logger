# SH2 (No-RTOS) DSF Logger over FTDI 

DSF Logger configures and communicates with the SensorHub (SH2) in SHTP over UART protocol through the FTDI interface. A group of sensors will be enabled based on the operating mode and rate specified. 
The output SH2 sensor reports will be saved to a log file in DSF format.

## Requirements

* SensorHub (BNO080, FSP200, etc.) with the FTDI Adapter 
* CMAKE 
* Visual Studio (for Windows) 

## Setup

* Clone this repository using the --recursive flag with git:
```
git clone --recursive http://github.hcrest.com/hillcrest/sh2-nortos-ftdi-dsf.git
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

## Building from Source

Run CMAKE to generate the makefile. CMAKE detects the platform which is run on (Windows/Linux) to generate the correct makefile.
```
cmake CMakeLists.txt
```

For Windows, open the generated solution (sh2_ftdi_logger.sln) in the Visual Studio to build the application.

For Linux, run MAKE to compile and build the application.
```
make -f Makefile
```

## Running the application
```
Usage: sh2_ftdi_logger.exe <*.json> [--template]
   *.json     - Configuration file in json format
   --template - Generate a configuration template file, 'logger.json'
```

### Generate Template Json File
Use the '--template' option to generate a batch json file template. The output file name is called "logger.json".
```
sh2_ftdi_logger.exe --template
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
sh2_ftdi_logger.exe logger.json
```

## License

TBD
