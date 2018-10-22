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
Usage: sh2_ftdi_logger.exe <out.dsf> [--deviceNumber=<id>] [--rate=<rate>] [--raw] [--calibrated] [--uncalibrated] [--mode=<9agm,6ag,6am,6gm,3a,3g,3m,all>] [--dcdAutoSave] [--calEnable=0x<mask>][--config]
   out.dsf              - output dsf file
   --deviceNumber=<id>  - which device to open.  0 for a single device.
   --rate=<rate>        - requested sampling rate for all sensors.  Default: 100Hz
   --raw                - include raw data and use SAMPLE_TIME for timing
   --calibrated         - include calibrated output sensors
   --uncalibrated       - include uncalibrated output sensors
   --mode=<mode>        - sensors types to log.  9agm, 6ag, 6am, 6gm, 3a, 3g, 3m or all.
   --dcdAutoSave        - enable DCD auto saving.  No dcd save by default.
   --clearDcd           - clear DCD and reset upon startup.
   --calEnable=0x<mask> - cal enable mask.  Bits: Planar, A, G, M.  Default 0x8
   --orientation=<orientation> - system orientation. enu, ned. Default: ned
   --batch=<filePath>   - batch file for the list of specific sensors in json format. When enabled, all other options are ignored. 
```

The list of sensor reports generated for each mode and options.

| Mode | Base Sensor Set | _Calibrated_ | _Uncalibrated_ | _Raw_ |
|---| --- | --- | --- | --- |
| **_all_** | ROTATION_VECTOR <br/> GAME_ROTATION_VECTOR <br/> GEOMAGNETIC_ROTATION_VECTOR | ACCELEROMETER <br/> GYROSCOPE_CALIBRATED <br/> MAGNETIC_FIELD_CALIBRATED | GYROSCOPE_UNCALIBRATED <br/> MAGNETIC_FIELD_UNCALIBRATED | RAW_ACCELEROMETER <br/> RAW_GYROSCOPE <br/> RAW_MAGNETOMETER |
| **_9agm_** | ROTATION_VECTOR  | ACCELEROMETER <br/> GYROSCOPE_CALIBRATED <br/> MAGNETIC_FIELD_CALIBRATED | GYROSCOPE_UNCALIBRATED <br/> MAGNETIC_FIELD_UNCALIBRATED | RAW_ACCELEROMETER <br/> RAW_GYROSCOPE <br/> RAW_MAGNETOMETER |
| **_6ag_** | GAME_ROTATION_VECTOR | ACCELEROMETER <br/> GYROSCOPE_CALIBRATED | GYROSCOPE_UNCALIBRATED | RAW_ACCELEROMETER <br/> RAW_GYROSCOPE |
| **_6am_** | GEOMAGNETIC_ROTATION_VECTOR | ACCELEROMETER <br/> MAGNETIC_FIELD_CALIBRATED | MAGNETIC_FIELD_UNCALIBRATED | RAW_ACCELEROMETER <br/> RAW_MAGNETOMETER |
| **_6gm_** |  | GYROSCOPE_CALIBRATED <br/> MAGNETIC_FIELD_CALIBRATED | GYROSCOPE_UNCALIBRATED <br/> MAGNETIC_FIELD_UNCALIBRATED | RAW_GYROSCOPE <br/> RAW_MAGNETOMETER |
| **_3a_** | | ACCELEROMETER | | RAW_ACCELEROMETER |
| **_3g_** | | GYROSCOPE_CALIBRATED | GYROSCOPE_UNCALIBRATED | RAW_GYROSCOPE |
| **_3m_** | | MAGNETIC_FIELD_CALIBRATED | MAGNETIC_FIELD_UNCALIBRATED | RAW_MAGNETOMETER |

### Examples 
Run DSF logger in '6ag' mode to enable the GameRV sensors along with the associated raw sensor reports. 
The SensorHub device is connected to /dev/ttyUSB2.

```
sh2_ftdi_logger.exe out_2.dsf --deviceNumber=2 --raw --mode=6ag
```

#### Batch Mode
Use the '--batch' option to generate a batch json file template. The output file name is called "logger.json".
```
sh2_ftdi_logger.exe --batch
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

To Run the DSF logger with the updated batch file
```
sh2_ftdi_logger.exe --batch=logger.json
```

## License

TBD
