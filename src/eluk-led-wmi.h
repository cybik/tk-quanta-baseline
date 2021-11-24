/*!
 * Copyright (c) 2021 Renaud Lepage <root@cybikbase.com>
 *
 * This file is inspired by tuxedo-keyboard, copyright (c) 2021 TUXEDO Computers.
 *
 * tk-quanta-baseline is free software: you can redistribute it and/or modify
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


// This WMI ID should be the conduit to write to ACPI_SMI, and read from it as well, 
//  via wmi_set_block/wmi_query_block.
#define ELUK_WMI_MGMT_GUID_LED_RD_WR  "644C5791-B7B0-4123-A90B-E93876E0DAAD"

// This WMI ID is an event conduit from which the system notifies the driver.
#define ELUK_WMI_EVNT_GUID_MESG_MNTR  "74286D6E-429C-427A-B34B-B5D15D032B05"

#define MODULE_ALIAS_ELUK_LED_WMI() \
    MODULE_ALIAS("wmi:" ELUK_WMI_MGMT_GUID_LED_RD_WR); \
    MODULE_ALIAS("wmi:" ELUK_WMI_EVNT_GUID_MESG_MNTR);

#define ELUK_LED_INTERFACE_WMI_STRID "eluk-led-wmi"

#define ELUK_WMI_LED_ZONE_LOGO    0x0008 // FIXME: doesn't do anything on Eluktronics. Probably for screen-light stuff? like models with cute designs on the back of the screen 
#define ELUK_WMI_LED_ZONE_TRUNK   0x0007 // TODO : actually do this blasted thing
#define ELUK_WMI_LED_ZONE_LEFT    0x0003
#define ELUK_WMI_LED_ZONE_CENTRE  0x0004
#define ELUK_WMI_LED_ZONE_RIGHT   0x0005

#endif