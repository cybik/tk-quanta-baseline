/*!
 * Copyright (c) 2021 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 * Parts of this file Copyright (c) 2021 Renaud Lepage <root@cybikbase.com>
 *
 * This file is part of eluk-wmi, an offshoot of tuxedo-keyboard, and heavily
 * modified to be nigh unrecognizable from the original. Notices will be
 * preserved per the spirit of Open Source development.
 ***************************************************************************
 * Modified Source Notice:
 * 
 * eluk-wmi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <https://www.gnu.org/licenses/>.
 *
 ***************************************************************************
 * Original Notice:
 * 
 * tuxedo-keyboard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ELUK_LED_INTERFACES_H
#define ELUK_LED_INTERFACES_H

#include "quanta_interfaces.h"


// This WMI ID should be the conduit to read and write to ACPI_SMI,
//  via wmi_set_block/wmi_query_block.
#define ELUK_WMI_MGMT_GUID_LED_RD_WR  "644C5791-B7B0-4123-A90B-E93876E0DAAD"

// This WMI ID is an event conduit from which the system notifies the driver.
#define ELUK_WMI_EVNT_GUID_MESG_MNTR  "74286D6E-429C-427A-B34B-B5D15D032B05"

#define MODULE_ALIAS_ELUK_LED_WMI() \
    MODULE_ALIAS("wmi:" ELUK_WMI_MGMT_GUID_LED_RD_WR); \
    MODULE_ALIAS("wmi:" ELUK_WMI_EVNT_GUID_MESG_MNTR);

#define ELUK_LED_IFACE_WMI_STRID "eluk-led-wmi"

// Zones
#define ELUK_WMI_LED_ZONE_LOGO             0x8 // FIX : Unused on Eluktronics
#define ELUK_WMI_LED_ZONE_TRUNK            0x7
#define ELUK_WMI_LED_ZONE_LEFT             0x5
#define ELUK_WMI_LED_ZONE_CENTRE           0x4
#define ELUK_WMI_LED_ZONE_RIGHT            0x3

// Off/Half/Full
#define ELUK_WMI_LED_BRIGHT_NONE           0x0
#define ELUK_WMI_LED_BRIGHT_HALF           0x1
#define ELUK_WMI_LED_BRIGHT_FULL           0x2

// Off/Half/Full
#define ELUK_WMI_LED_EFFECT_SOLID          0x1
#define ELUK_WMI_LED_EFFECT_BREATHE        0x3
#define ELUK_WMI_LED_EFFECT_RAINBOW        0x6
#define ELUK_WMI_LED_EFFECT_AMBIENT        0x7

//#define ELUK_WMI_LED_BREF_

//#define ELUK_DEBUGGING 1


#endif