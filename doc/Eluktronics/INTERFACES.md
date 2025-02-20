# Eluktronics Prometheus XVI data

The following information is what the `eluk_led_interfaces.h` and `eluk-*` files are basing themselves on to
attempt interaction with the WMI interface. This has been documented as a reference point.

## Hey this looks legit

It is **EXTREMELY NOT**. This data is the result of sleuthing and sniffing around personally; @cybik (the author of the Eluktronics integration at this time) is not related to Eluktronics in any way,
and only owns hardware sold by Eluktronics.

I repeat: this work is completely unrelated to Eluktronics, and is a fully user/developer-driven, external development effort. There is no claim of anything being "official".

## What is this anyway?

The data contained here should help you poke at an Eluktronics Prometheus XVI's RW_GMWMI interface with a chance of it not exploding.

## Doesn't look like Windows stuff to me

Look at the ControlCenter app with dotPeek or Jetbrains Rider and thank me later.

## "chance of it not exploding" sounds ominous.

This is research-type data to *develop* Linux support for hardware. It is not intended for users, so yeah, here there be dragons.

## I'm a dev and I have this hardware. How do I use it?

`wmi_query_block` and `wmi_set_block`. If you're already out of your depth here, abandon all hope. Again, this is technical details, not user stuff.

# Data

Note that `0xFA00 == 64000` and `0xFB00 == 64256`. Hex are regularly used in coding, but the decompile
showed decimal units, so when looking at decomp, look for the decimal values above.

### Get

|Operation|Settings|Outcome|Notes|
|--- |--- |--- |--- |
|Get LED status?|a0 = 0xFA00<br/>a1 = 0x0100|If nothing is put aside from a1 and a2, it seems as if there is a returned value. check.|Per spec, a2 contains the RGB setting, but might be only one? or memory shenanigans for everything? this one is danger.|
|Get Hardware Info|a0 = 0xFA00<br/>a1 = 0x0200|Data available? Check if can be read immediately post.<br/>a2: CPU Temp, a3: GPU Temp, a4: CPU fan speed, a5: GPU fan speed, a6: Monitor alpha? might be brightness?|Temps unnecessary, fan speeds could be interesting.<br/>After checking, Monitor Alpha is actually for monitoring the current state of the keyboard "alpha" (brightness)|
|Magic Check|a0 = 0xFA00<br/>a1 = 0x0201|Data available? Check if can be read immediately post. a0 would become the bios version post smi on Windows?|Interestingly, this also returns keyboard states in a2. Sleuth more to reverse this properly.|
|Unknown Check|a0 = 0xFA00<br/>a1 = 0x0202|Dumps STTP into a2. Related to TPON/TPOF?|This is retroed from a decompile of an ACPI dump.|
|Unknown Check|a0 = 0xFA00<br/>a1 = 0x0300|Possibly dumps/informs of a power management state?|This is retroed from a decompile of an ACPI dump.|

### Set

|Operation|Settings|Outcome|Notes|
|--- |--- |--- |--- |
|Set LEDs|a0 = 0xFB00<br/>a1 = 0x0100<br/>a2 = zone<br/>a3 = colors|Magic values: a2 = 6 to set all zones; a2 = 0 to set all devices?|Colors are in #AARRGGBB format (bytes inverted, little endian)|
|Set "Win Key"|a0 = 0xFB00<br/>a1 = 0x0200<br/>a6 = bool|a6 to 0 to disable the Win key, 1 to enable the Win key.|There seems to be no way to check from code (per the Quanta app)|
|Unknown Set|a0 = 0xFB00<br/>a1 = 0x0202<br/>a2 = 0 or 1, presumably a bool|Unknown|Sets PCI0.SBRG.EC0.ECWT to TPOF or TPON|
|Set "Power Mode"|a0 = 0xFB00<br/>a1 = 0x0300<br/>a2 = some numbers|Unknown|Unknown|

For the record, it seems that no check methods will output the keyboard light settings.

## 0x0201 Magic Check

The magic check needs a query right after the set. That being said, here's what I could decipher:

* a0 is a BIOS Version
  * It does NOT match the DMI version
  * A check here could be useful for functionality shielding (>=260)
  * Decide on either DMI or WMI hardcheck
    * Or both, whatev
* a1 is unused?
* a2 seems to be a variant ID

### LED Magic Check

The `a0 = 0xFA00` / `a1 = 0x0201` combo seems to be a bit more than suspected at first. Looking at
the decompile, this set/get seems to be used to "unlock" features based off of A2'S returned value:

|Value|Behaviour|
|--- |---|
|< 10|FU|
|10|LED zones 1, 2, 3 visible. Logo zone enabled?|
|12 or 14|page2_LED visible|
|13|LED Logo visible, also keyboard?|
|15|LED Logo and Trunk visible, also keyboard?|

NotHaier behaviour seems limited to the Casper G911 unit and only changes the Logo config.

Update: LED magic check seems useless after test.
