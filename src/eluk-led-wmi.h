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
#define ELUK_WMI_LED_ZONE_LOGO            0x08 // FIX : Unused on Eluktronics
#define ELUK_WMI_LED_ZONE_TRUNK           0x07
#define ELUK_WMI_LED_ZONE_LEFT            0x05
#define ELUK_WMI_LED_ZONE_CENTRE          0x04
#define ELUK_WMI_LED_ZONE_RIGHT           0x03

// Known effect/brightness Settings ("alpha" channels)
#define ELUK_WMI_LED_BREF_SOLID_OFF       0x00 // 0? not -1? eh. validated.
#define ELUK_WMI_LED_BREF_SOLID_HALF      0x11 // valid for trunk also
#define ELUK_WMI_LED_BREF_SOLID_FULL      0x12 // valid for trunk also

// Soothing breathing pattern
#define ELUK_WMI_LED_BREF_BREATHING_OFF   0x30 //?
#define ELUK_WMI_LED_BREF_BREATHING_HALF  0x31
#define ELUK_WMI_LED_BREF_BREATHING_FULL  0x32

// Reminescent of the rainbow road! (Known as Colorful Cycle)
#define ELUK_WMI_LED_BREF_RAINBOW_OFF     0x60
#define ELUK_WMI_LED_BREF_RAINBOW_HALF    0x61
#define ELUK_WMI_LED_BREF_RAINBOW_FULL    0x62

// AMBILIGHT IS ONLY FOR THE KEYBOARD.
#define ELUK_WMI_LED_BREF_AMBILIGHT_OFF   0x70
#define ELUK_WMI_LED_BREF_AMBILIGHT_HALF  0x71
#define ELUK_WMI_LED_BREF_AMBILIGHT_FULL  0x72

//#define ELUK_WMI_LED_BREF_

// Baselines / Defaults.
//  a2: zone; a3: color
union wmi_setting offline_union[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LOGO,   .a3 = 0x0000FFFF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // trunk/logo?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_TRUNK,  .a3 = 0x0000FFFF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // logo/trunk?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_RIGHT,  .a3 = 0x000000FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led3 - right
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_CENTRE, .a3 = 0x0000FF00, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led2 - centre
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LEFT,   .a3 = 0x00FF0000, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led1 - left
};

union wmi_setting solid_50_union[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LOGO,   .a3 = 0x1000FFFF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // trunk/logo?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_TRUNK,  .a3 = 0x110022FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // logo/trunk?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_RIGHT,  .a3 = 0x110000FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led3 - right
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_CENTRE, .a3 = 0x1100FF00, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led2 - centre
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LEFT,   .a3 = 0x11FF0000, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led1 - left
};

union wmi_setting solid_100_union[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LOGO,   .a3 = 0x1000FFFF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // trunk/logo?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_TRUNK,  .a3 = 0x120022FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // logo/trunk?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_RIGHT,  .a3 = 0x120000FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led3 - right
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_CENTRE, .a3 = 0x1200FF00, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led2 - centre
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LEFT,   .a3 = 0x12FF0000, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led1 - left
};

union wmi_setting breathing_50_union[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LOGO,   .a3 = 0x1000FFFF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // trunk/logo?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_TRUNK,  .a3 = 0x310022FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // logo/trunk?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_RIGHT,  .a3 = 0x310000FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led3 - right
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_CENTRE, .a3 = 0x3100FF00, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led2 - centre
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LEFT,   .a3 = 0x31FF0000, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led1 - left
};

union wmi_setting breathing_100_union[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LOGO,   .a3 = 0x1000FFFF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // trunk/logo?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_TRUNK,  .a3 = 0x320022FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // logo/trunk?
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_RIGHT,  .a3 = 0x320000FF, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led3 - right
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_CENTRE, .a3 = 0x3200FF00, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led2 - centre
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, .a2 = ELUK_WMI_LED_ZONE_LEFT,   .a3 = 0x32FF0000, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, // led1 - left
};

#endif