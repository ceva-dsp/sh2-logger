# SH2 Logger for SH2 devices

The SH2 Logger is a command line utility that enables logging sensor data from a sensor hub over USB (FTDI UART interface).

A group of sensors will be enabled based on the operating mode and rate specified. 
The output SH2 sensor reports will be saved to a log file in DSF format.

This utility can also be used to perform firmware updates.

## Requirements

* SensorHub (BNO080, FSP200, MotionEngine Scout Evaluation Module,
  etc.) with an FTDI UART-USB Adapter 
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
  * For Linux, you can add a .rules file (e.g. `99-usb-serial.rules`) under `/etc/udev/rules.d` with
    the following: 
```
ACTION=="add", SUBSYSTEM=="usb-serial", DRIVER=="ftdi_sio", ATTR{latency_timer}="1"
```

## Building from Source

CMake generates the build configuration for your development platform. From the root of this
project, run

```
cmake -B build 
```
to generate a `build` directory for your platform. 

To build the application, run
```
cmake --build build
```

This build has been tested on Windows with Visual Studio 16 2019 (32-
and 64-bit targets) and on x86-64 and ARM Linux with gcc 9.3.0.

## Running the application

The application can be run in several modes. The most typical usage is
to generate a configuration file which dictates what sensors are to be
activated and what rates, then to run the application in logger mode
to collect data.

Run the resulting binary with the `--help` flag to print complete usage
details.

```
./sh2_logger.exe --help

USAGE:

   path\to\your\sh2-logger\build\Debug\sh2_logger.exe [-w
                                        <wheel_source>] [--clearOfCal <0
                                        |1>] [--clearDcd <0|1>] [-d
                                        <device-name>] [-o <filename>] [-i
                                        <filename>] [--] [--version] [-h]
                                        <log|dfu-bno|dfu-fsp200|template>


Where: 

   -w <wheel_source>,  --wheel_source <wheel_source>
     Wheel data source. - for stdin

   --clearOfCal <0|1>
     Clear optical flow calibration at logger start. Overrides setting in
     configuration file if provided, otherwise defaults to 0 (do not
     clear).

   --clearDcd <0|1>
     Clear dynamic IMU calibration at logger start. Overrides setting in
     configuration file if provided, otherwise defaults to 0 (do not
     clear).

   -d <device-name>,  --device <device-name>
     Serial port device (For Windows, FTDI device number, usually 0.)

   -o <filename>,  --output <filename>
     Output filename (sensor .dsf log for 'log' command, logger .json
     configuration for 'template' command)

   -i <filename>,  --input <filename>
     Input filename (configuration for 'log' command, firmware file for
     DFU)

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

   <log|dfu-bno|dfu-fsp200|template>
     (required)  Operation to perform


   SH2 Logging utility

```

### Typical Dead Reckoning (ME-Scout) configuration

A configuration file for 100 Hz odometry output is provided as
`dr100.json`. This is suitable for use with an ME-Scout evaluation
module.  

If your module is configured to read wheel encoder data from the host
interface, the file from which wheel position data will be read is
specified with the `-w` option (use `-w -` to read data from stdin).
See `FileWheelSource.h` for details of the expected data format.  Note
that the application may block at startup until the input file can be
opened.


### Custom Configuration File
Use the 'template' subcommand to generate a json configuration file template. The output file name is specified with the `-o` option.

```
sh2_logger.exe template -o config.json
```

Next, modify the generated json file. To enable sensors, specify the operating rate of each active sensor in the json file. 
For instance, to enable the GameRV sensors at 100Hz and the Accelerometer at 200Hz. Update the "sensorList" section of the json file as below.
```
{
    "calEnable": "0x00",
    "clearDcd": false,
    "clearOfCal": false,
    "dcdAutoSave": false,
    "orientation": "ned",
    "rawSampleTime": false,
    "sensorList": {
        "ARVR Stabilized GameRotation Vector": 0,
        "ARVR Stabilized Rotation Vector": 0,
        "Accelerometer": 200,
        "Ambient Light": 0,
        "Circle Detector": 0,
		"Dead Reckoning Pose": 0,
        "Flip Detector": 0,
        "Game Rotation Vector": 100,
        "Geomagnetic Rotation Vector": 0,
        "Gravity": 0,
		....
```

Additional configurations:
 - The `calEnable` field controls the enabled calibration modes. It is
   a hexadecimal value made by bitwise OR of the following:
   - 0x08: Enable enhanced planar-motion accelerometer calibration.
   - 0x01: Enable 3-D accelerometer calibration
   - 0x02: Enable 3-D gyroscope calibration
   - 0x04: Enable 3-D magnetometer calibration
   - This value should be set to 0x08 for robotics applications that
     use primarily planar motion (e.g. robot vacuum cleaners,
     warehouse robots). 
 - `clearDcd`: When 'true', clear Dynamic Calibration Data for the IMU
   when starting up the application.
 - `clearOfCal`: When 'true', clear calibration data for the optical
   flow sensor when starting up the application.
 - `dcdAutoSave`: When 'true', the Dynamic Calibration Data learned by
   the IMU will be saved to non-volatile flash memory periodically. In
   some cases, enabling this may cause short-term disruption during
   periodic flash-writes.
 - `orientation`: specifies axis conventions for non-raw output data.
   "enu" (X = East/Right, Y = North/Forward, Z = Up) and "ned" (X =
   North/Forward, Y = East/Right, Z = Up) are supported.
 - Some sensors may have additional `sniffEnabled` and `sensorSpecific`
   options.
   - `sensorSpecific` behavior varies by sensor.    
   - `sniffEnabled` indicates that the logger will not attempt to
     configure the sensor, but will output data from it if it is
     activated (e.g. as a dependency of another output).



### Data Collection

#### Running data logger
To log data, provide a configuration file with the `-i` parameter, 
a destination output file with the `-o` parameter, and a device
identifier with the `-d` parameter. 

In Windows builds, the argument to `-d` is 0 when there is a single
FTDI UART-USB adapter connected -- this will typically be the case. In
the event that multiple modules are connected to the same Windows
host, 0 is the first one that was connected and 1 is the second (and
so on). 

```
sh2_logger.exe log -i <config>.json -o <output>.dsf -d 0
```

In Linux builds, the argument to `-d` is the path to the device file.
This is typically either `/dev/ttyUSB0` when a single FTDI UART-USB
adapter is connected or to one of the entries under
`/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_*-if00-port0`. 


If desired, one can set up a Linux udev rule to simplify
identification.  Following the example above for setting the latency
timer, the following line will automatically create a symlink at
`/dev/imu_0` when the FTDI UART-USB adapter having serial number 
"DK000000" is plugged in:


```
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6015", ATTRS{serial}=="DK000000", SYMLINK+="imu_0"
```


## Download Firmware Update

This application can also be used to update the firmware on compatible
CEVA devices. 

To obtain the necessary firmware file, contact CEVA, Inc., Sensor Fusion
Business Unit, about licensing terms.

### Connection

The DTR signal from the FTDI interface must be connected to RESETN on the BNO08x.

The RTS signal from the FTDI interface must be connected to BOOTN on the BNO08x.

Alternatively, GPIO pins can be used to control these interface lines
if available for your platform. Modify the functions setResetN() and
setBootN() in the appropriate files for your platform:

 - `hal/bno_dfu_hal_Linux.c`
 - `hal/bno_dfu_hal_Win.c`
 - `hal/ftdi_hal_Linux.c`
 - `hal/ftdi_hal_Win.c`



### Running the DFU process.

The following commands are run to install a firmware image on a
compatible device.  Users of the BNO series of products should use the
`dfu-bno` command, while suers of the FSP200 products should use
`dfu-fsp200`. Consult with CEVA's Sensor Fusion Business Unit for
details.

```
./sh2_logger dfu-fsp200 -i <path_to_firmware.hcbin> -d <device> 
```
or
```
./sh2_logger dfu-bno -i <path_to_firmware.hcbin> -d <device> 
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

