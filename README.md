STM32F103 Dual gang non-stop rotary pot
----

This is a small project only for testing a dual gang free-spinning rotary
pot I got my hands on. The interesting thing with this pot is that the two
wipers (one for each internal resistor) have a `90 degrees phase` between
each other. Therefore, the code logic is based this difference.

The beautiful and awesome ascii graphics below show the Vadc on the two
wipers of the pot while the pot is turning. So, if you have a 12-bit ADC
then the max value will be in the range of [0-4095) ((1<12)-1=4095), which
means that `min=0` and `max=4095`. Each wiper is sampled with using a different
ADC channel `ADC1` and `ADC2`.

The logic of the code is just to detect the direction where the pot is
turning and increment its value when it's turned right and decrement
when it's turned left. To do this we split the below graph in to 4 main
quarters, each for 1/4 of the half period (aka 1 full turn of the pot).
With this trick by comparing two sequential ADC values of ADC1 and ADC2
we can detect the exact location of the pot and also in which direction
is turning.

To be more specific, in `Q1` we know that `ADC1` is in the range `[min-1/2]`
as also `ADC2`. But by comparing two different ADC samples, then we can
detect if the pot is turned right or left. Therefore, in `Q1` if `ADC1` is
increment (+) and `ADC2` is decrement (-), then we know that the pot is
turned left. And if `ADC2=(+)` and `ADC1=(-)` then is turned right.

```
           |1|2|3|4|1|2|3|4|
      max  _________________
               /\      /\
ADC2  1/2  ___/__\___ /__\___
             /    \  /    \
           _/______\/______\_
      min

      max  ____________________
                 /\      /\
ADC1  1/2  _____/__\___ /__\___
            \  /    \  /    \
           __\/______\/______\_
      min
```

The code also supports dead-zone for each individual (ADC/wiper). The dead
zone is used for two reasons. The one is the to minimize ADC noise and the
other is to increase or decrease the pot sensitivity. Dead-zone means that
the next sampled value must be larger or smaller than the current by the
dead-zone value. Therefore, if the current ADC value is `1000` and the dead
zone value is set to `30`, then to actually run the calculation algorithm
the next sample but be `>=130` or `<=70`. That also means that this 60-bit dead
zone will affect the pot sensitivity, which means that you need to do more
turns to get from the relative min to relative max value.

The algorithm is based on relative values and supports both signed and
unsigned integers and also floats if the MCU is capable. Also the init code
can set the initial value. Also, each pot can have individual settings.
Therefore, one pot and only have integer values in the range of `[0-111]` with
initial value of `72`. Another have range `[-100,100]` with initial value `0` and
another one `[-120.25,70.50]` with initial value of `2.25`. Also, each different
pot supports an individual step, which also can be integer of float.

Therefore, the pot API provides individual range, min, max, step and dead-zone
values and you can use any number of pots as long you initialize the proper
size in the init function.


### How to compile and flash
You need cmake to build this project either on Windows or Linux.
To setup the cmake properly
follow the instructions from [here](https://bitbucket.org/dimtass/cmake_toolchains/src/master/README.md).
Then edit the `cmake/TOOLCHAIN_arm_none_eabi_cortex_m3.cmake` file
and point `TOOLCHAIN_DIR` to the correct GCC path.

e.g. on Windows
```sh
set(TOOLCHAIN_DIR C:/opt/gcc-arm-none-eabi-4_9-2015q3-20150921-win32)
```

or on Linux
```sh
set(TOOLCHAIN_DIR /opt/gcc-arm-none-eabi-4_9-2015q3)
```

Then on Windows run ```build.cmd``` or on Linux run ```./build.bash```
and the .bin and .hex files should be created in the ```build-stm32/src```
folder. Also, a .cproject and .project files are created if you want to
edit the source code.

To flash the HEX file in windows use st-link utility like this:
```"C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe" -c SWD -p build-stm32\src\stm32f103-dual-gang-pot.hex -Rst```

To flash the bin in Linux:
```st-flash --reset write build-stm32/src/stm32f103-dual-gang-pot.bin 0x8000000```

## FW details
* `CMSIS version`: 5.3.0
* `StdPeriph Library version`: 3.6.1

