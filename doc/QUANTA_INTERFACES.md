# Eluktronics Prometheus XVI data

The following information is what the quanta_interfaces.h and eluk-led files are basing themselves on to
attempt interaction with the WMI interface. This has been documented as a reference point.

|Operation|Settings|Outcome|
|--- |--- |--- |
|Get LED status?|a0 = 0xFA00 a1 = 0x0100|If nothing is put aside from a1 and a2, it seems as if there is a returned value. check.<br/>Per spec, a2 contains the RGB setting, but might be only one? or memory shenanigans for everything? this one is danger.|
|Get Hardware Info|a0 = 0xFA00   a1 = 0x0200|Data available? Check if can be read immediately post.<br/>a2: CPU Temp, a3: GPU Temp, a4: CPU fan speed, a5: GPU fan speed, a6: Monitor alpha? might be brightness?Temps unnecessary, fan speeds could be interesting.|
|BIOS Version Check|a0 = 0xFA00   a1 = 0x0201|Data available? Check if can be read immediately post. a0 would become the bios version post smi on Windows?<br/>Interestingly, this also returns keyboard states in a2. Sleuth more to reverse this properly.|
|Set LEDs|a0 = 0xFB00  a1 = 0x0100<br/>a2 = zone      a3 = colors|Colors are in #AARRGGBB format (bytes inverted, little endian)<br/>Magic values: a2 = 6 to set all zones; a2 = 0 to set all devices?|
|Set "Win Key"|a0 = 0xFB00  a1 = 0x0200<br/>a6 = bool|a6 to 0 to disable the Win key, 1 to enable the Win key.<br/>Note: there seems to be no way to check from code (per the Quanta app)|
|Set "Power Mode"|a0 = 0xFB00    a1 = 0x0300<br/>a2 = some numbers|Unknown|
