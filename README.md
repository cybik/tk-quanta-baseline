# Table of Contents

- [Notice](#notice)
  - [Hey this looks legit](#legit)
  - [What are you doing here?](#nani)
  - [Why tho](#doushite)
- <a href="#description">Description</a>
- <a href="#building">Building and Install</a>
- <a href="#using">Using</a>
- <a href="#sysfs">Sysfs</a>
- <a href="#kernelparam">Kernel Parameter</a>
- <a href="#modes">Modes</a>

# IMPORTANT - Forked Repository Notice <a name="notice"></a>
This forked repository is highly experimental, highly verbose, and should not be considered
official in any capacity.

## Hey this looks legit <a name="legit"></a>

It is **EXTREMELY NOT**. This data is the result of sleuthing and sniffing around personally; @cybik (the author of the Eluktronics integration at this time) is not related to Eluktronics in any way,
and only owns hardware sold by Eluktronics.

I repeat: this work is completely unrelated to Eluktronics, and is a fully user/developer-driven, external development effort. There is no claim of anything being "official".

## What are you doing here? <a name="nani"></a>
This repository fork uses upstream's clevo_wmi code as a jumping point to start poking at hooking
into the Eluktronics Prometheus XVI's WMI interfaces to access and, ultimately, control the settings
for the keyboard and fan LEDs.

## Why tho <a name="doushite"></a>
Literally the only thing that's "not working" on this laptop is LED controls. Everything else just
worked out of the box after upgrading the laptop to 5.15. --@cybik

# Description <a name="description"></a>
TUXEDO Computers kernel module drivers for keyboard, keyboard backlight & general hardware I/O

Features:

- Driver for Fn-keys
- Sysfs control of brightness/color/mode for most TUXEDO keyboards (note: white backlight only models are currently not supported)
- Hardware I/O driver for TUXEDO Control Center

Modules included in this package:

- tuxedo-keyboard
- tuxedo-io
- clevo-wmi
- clevo-acpi

Additional modules not related to Tuxedo products:

- eluk-led-wmi

# Building and Installing <a name="building"></a>

## Dependencies

- make
- gcc
- linux-headers
- dkms (Only when using this module with DKMS functionality)

## Warning when installing modules

Use either method only. Do not combine installation methods, such as starting with the build step below and proceeding to use the same build artifacts with the DKMS module. Otherwise the module built via dkms will fail to load with an `exec_format` error on newer kernels due to a mismatched version magic.

This is why the DKMS build step begins with a `make clean` step.

For convenience, on platforms where DKMS is in use, skip to the DKMS section directly.

## Cloning the Git Repo

```sh
git clone https://github.com/tuxedocomputers/tuxedo-keyboard.git

cd tuxedo-keyboard

git checkout release
```

## Building the Module(s)

```sh
make clean && make
```

## The DKMS route

### Add as DKMS Module(s)

Install the Module:

```sh
make clean

sudo make dkmsinstall
```

Load the Module with modprobe:

```sh
modprobe tuxedo_keyboard
```

or

```sh
sudo modprobe tuxedo_keyboard
```

You might also want to activate `tuxedo_io` module the same way if you are using [TCC](https://github.com/tuxedocomputers/tuxedo-control-center).

### Uninstalling DKMS module(s)

Remove the DKMS module and source:

```sh
sudo make dkmsremove

sudo rm /etc/modprobe.d/tuxedo_keyboard.conf
```

# Using <a name="using"></a>

## modprobe

```sh
modprobe tuxedo_keyboard
```

## Load Module(s) on boot

If a module is relevant it will be loaded automatically on boot. If it is not loaded after a reboot, it most likely means that it is not needed.

Add Module to /etc/modules:

```sh
sudo su

echo tuxedo_keyboard >> /etc/modules
```

Default Parameters at start.


In this example, we start the kernel module with the following settings:

- mode 0 (Custom / Default Mode)
- red color for the left side of keyboard
- green color for the center of keyboard
- blue color for the right side of keyboard

Note that we write it's settings to a `.conf` file under `/etc/modprobe.d` named `tuxedo_keyboard.conf`.

```sh
sudo su

echo "options tuxedo_keyboard mode=0 color_left=0xFF0000 color_center=0x00FF00 color_right=0x0000FF" > /etc/modprobe.d/tuxedo_keyboard.conf
```

or

```sh
sudo cp tuxedo_keyboard.conf /etc/modprobe.d/tuxedo_keyboard.conf
```

# Sysfs <a name="sysfs"></a>

## General

Path: /sys/devices/platform/tuxedo_keyboard

## color_left

Allowed Values: Hex-Value (e.g. 0xFF0000 for the Color Red)
Description: Set the color of the left Keyboard Side

## color_center

Allowed Values: Hex-Value (e.g. 0xFF0000 for the Color Red)
Description: Set the color of the center of Keyboard

## color_right

Allowed Values: Hex-Value (e.g. 0xFF0000 for the Color Red)
Description: Set the color of the right Keyboard Side

## color_extra

Allowed Values: Hex-Value (e.g. 0xFF0000 for the Color Red)
Description: Set the color of the extra region (if exist) of the Keyboard

## brightness

Allowed Values: 0 - 255
Description: Set the brightness of the Keyboard

## mode

Allowed Values: 0 - 7
Description: Set the mode of the Keyboard. A list with the modes is under <a href="#modes">Modes</a>

## state

Allowed Values: 0, 1
Description: Set the State of keyboard, 0 is keyboard is off and 1 is keyboard is on

## extra

Allowed Values: 0, 1
Description: Only get the information, if the keyboard have the extra region

# Kernel Parameter <a name="kernelparam"></a>

## Using

```sh
sudo modprobe tuxedo_keyboard <params>
```

## color_left

Set the color of the left Keyboard Side

## color_center

Set the color of the left Keyboard Side

## color_right

Set the color of the left Keyboard Side

## color_extra

Set the color of the left Keyboard extra region (Only when is a supported keyboard)

## mode

Set the mode (on/off) of keyboard

## brightness

Set the brightness of keyboard

## state

Sets the state?

# Modes <a name="modes"></a>

## CUSTOM

Value: 0

## BREATHE

Value: 1

## CYCLE

Value: 2

## DANCE

Value: 3

## FLASH

Value: 4

## RANDOM_COLOR

Value: 5

## TEMPO

Value: 6

## WAVE

Value: 7
