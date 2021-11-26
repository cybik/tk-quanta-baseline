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

|Operation|Settings|Outcome|Notes|
|--- |--- |--- |--- |
|Get LED status?|a0 = 0xFA00<br/>a1 = 0x0100|If nothing is put aside from a1 and a2, it seems as if there is a returned value. check.|Per spec, a2 contains the RGB setting, but might be only one? or memory shenanigans for everything? this one is danger.|
|Get Hardware Info|a0 = 0xFA00<br/>a1 = 0x0200|Data available? Check if can be read immediately post.<br/>a2: CPU Temp, a3: GPU Temp, a4: CPU fan speed, a5: GPU fan speed, a6: Monitor alpha? might be brightness?|Temps unnecessary, fan speeds could be interesting.|
|BIOS Version Check|a0 = 0xFA00<br/>a1 = 0x0201|Data available? Check if can be read immediately post. a0 would become the bios version post smi on Windows?|Interestingly, this also returns keyboard states in a2. Sleuth more to reverse this properly.|
|Set LEDs|a0 = 0xFB00<br/>a1 = 0x0100<br/>a2 = zone<br/>a3 = colors|Magic values: a2 = 6 to set all zones; a2 = 0 to set all devices?|Colors are in #AARRGGBB format (bytes inverted, little endian)|
|Set "Win Key"|a0 = 0xFB00<br/>a1 = 0x0200<br/>a6 = bool|a6 to 0 to disable the Win key, 1 to enable the Win key.|There seems to be no way to check from code (per the Quanta app)|
|Set "Power Mode"|a0 = 0xFB00<br/>a1 = 0x0300<br/>a2 = some numbers|Unknown|Unknown|
