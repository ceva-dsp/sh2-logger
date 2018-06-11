# SH2 (No-RTOS) DSF Logger over FTDI 

DSF Logger configures and communicates with the SensorHub (SH2) in SHTP over UART protocol through the FTDI interface. A group of sensors will be enabled based on the operating mode and rate specified. 
The output SH2 sensor reports will be saved to a log file in DSF format. 

## Getting Started

### Prerequisites

Clone the repository along with the 'sh2' library submodule to the local machine.
Install CMAKE

### Compile and Build from Source

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
Usage: sh2_ftdi_logger.exe <out.dsf> [--deviceNumber=<id>] [--rate=<rate>] [--raw] [--calibrated] [--uncalibrated] [--mode=<9agm,6ag,6am,6gm,3a,3g,3m,all>] [--dcdAutoSave] [--calEnable=0x<mask>]
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
```


| Mode | Base Sensor Set | _Calibrated_ | _Uncalibrated_ | _Raw_ |
|---| --- | --- | --- | --- |
| **_all_** | ROTATION_VECTOR <br/> GAME_ROTATION_VECTOR <br/> GEOMAGNETIC_ROTATION_VECTOR | SH2_ACCELEROMETER <br/> SH2_GYROSCOPE_CALIBRATED <br/> SH2_MAGNETIC_FIELD_CALIBRATED | SH2_GYROSCOPE_UNCALIBRATED <br/> SH2_MAGNETIC_FIELD_UNCALIBRATED | SH2_RAW_ACCELEROMETER <br/> SH2_RAW_GYROSCOPE <br/> SH2_RAW_MAGNETOMETER |
| **_9agm_** | ROTATION_VECTOR  | SH2_ACCELEROMETER <br/> SH2_GYROSCOPE_CALIBRATED <br/> SH2_MAGNETIC_FIELD_CALIBRATED | SH2_GYROSCOPE_UNCALIBRATED <br/> SH2_MAGNETIC_FIELD_UNCALIBRATED | SH2_RAW_ACCELEROMETER <br/> SH2_RAW_GYROSCOPE <br/> SH2_RAW_MAGNETOMETER |
| **_6ag_** | GAME_ROTATION_VECTOR | SH2_ACCELEROMETER <br/> SH2_GYROSCOPE_CALIBRATED | SH2_GYROSCOPE_UNCALIBRATED | SH2_RAW_ACCELEROMETER <br/> SH2_RAW_GYROSCOPE |
| **_6am_** | GEOMAGNETIC_ROTATION_VECTOR | SH2_ACCELEROMETER <br/> SH2_MAGNETIC_FIELD_CALIBRATED | SH2_MAGNETIC_FIELD_UNCALIBRATED | SH2_RAW_ACCELEROMETER <br/> SH2_RAW_MAGNETOMETER |
| **_6gm_** |  | SH2_GYROSCOPE_CALIBRATED <br/> SH2_MAGNETIC_FIELD_CALIBRATED | SH2_GYROSCOPE_UNCALIBRATED <br/> SH2_MAGNETIC_FIELD_UNCALIBRATED | SH2_RAW_GYROSCOPE <br/> SH2_RAW_MAGNETOMETER |
| **_3a_** | | SH2_ACCELEROMETER | | SH2_RAW_ACCELEROMETER |
| **_3a_** | | SH2_GYROSCOPE_CALIBRATED | SH2_GYROSCOPE_UNCALIBRATED | SH2_RAW_GYROSCOPE |
| **_3m_** | | SH2_MAGNETIC_FIELD_CALIBRATED | SH2_MAGNETIC_FIELD_UNCALIBRATED | SH2_RAW_MAGNETOMETER |

### Examples 

Run DSF logger in '6ag' mode to enable the GameRV sensors along with the associated raw sensor reports. 
The SensorHub device is connected to /dev/ttyUSB2.

```
sh2_ftdi_logger.exe out_2.dsf --deviceNumber=2 --raw --mode=6ag
```


## License

TBD
