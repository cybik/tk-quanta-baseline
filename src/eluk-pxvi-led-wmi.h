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

#define ELUK_LED_IFACE_WMI_STRID "eluk-pxvi-led-wmi"

// Zones
#define ELUK_WMI_LED_ZONE_LOGO             0x8 // FIX : Unused on Eluktronics
#define ELUK_WMI_LED_ZONE_TRUNK            0x7
#define ELUK_WMI_LED_ZONE_RIGHT            0x3
#define ELUK_WMI_LED_ZONE_CENTRE           0x4
#define ELUK_WMI_LED_ZONE_LEFT             0x5

// Off/Half/Full
#define ELUK_WMI_LED_BRIGHT_NONE           0x0
#define ELUK_WMI_LED_BRIGHT_HALF           0x1
#define ELUK_WMI_LED_BRIGHT_FULL           0x2

// Off/Half/Full
#define ELUK_WMI_LED_EFFECT_SOLID          0x1
#define ELUK_WMI_LED_EFFECT_BREATHE        0x3
#define ELUK_WMI_LED_EFFECT_RAINBOW        0x6
#define ELUK_WMI_LED_EFFECT_AMBIENT        0x7

// Special Magicks
#define ELUK_WMI_LED_COLOR_AMBIENT         0x101010

//#define ELUK_WMI_LED_BREF_
#define ELUK_ENABLE_PRESETS

typedef void (eluk_led_evt_cb_int_t)(u32);
typedef void (eluk_led_evt_cb_buf_t)(u8, u8*);

struct eluk_led_interface_t {
    char *string_id;
    eluk_led_evt_cb_int_t *evt_cb_int;
    eluk_led_evt_cb_buf_t *evt_cb_buf;
};

u32 eluk_led_add_interface(const char* name, struct eluk_led_interface_t *new_interface);
u32 eluk_led_remove_interface(const char* name, struct eluk_led_interface_t *interface);
u32 eluk_led_get_active_interface_id(char **id_str);

static struct eluk_led_interfaces_t {
    struct eluk_led_interface_t *wmi;
} eluk_led_interfaces = { .wmi = NULL };

eluk_led_evt_cb_int_t eluk_led_evt_cb_int;
eluk_led_evt_cb_buf_t eluk_led_evt_cb_buf;


static DEFINE_MUTEX(eluk_led_interface_modification_lock);

u32 eluk_led_add_interface(const char* interface_name,
                         struct eluk_led_interface_t *interface)
{
    mutex_lock(&eluk_led_interface_modification_lock);

    if (strcmp(interface->string_id, interface_name) == 0) {
        eluk_led_interfaces.wmi = interface;
    } else {
        TUXEDO_DEBUG("trying to add unknown interface\n");
        mutex_unlock(&eluk_led_interface_modification_lock);
        return -EINVAL;
    }
    interface->evt_cb_int = eluk_led_evt_cb_int;
    interface->evt_cb_buf = eluk_led_evt_cb_buf;

    mutex_unlock(&eluk_led_interface_modification_lock);

    return 0;
}
EXPORT_SYMBOL(eluk_led_add_interface);

u32 eluk_led_remove_interface(const char* interface_name, 
                            struct eluk_led_interface_t *interface)
{
    mutex_lock(&eluk_led_interface_modification_lock);

    if (strcmp(interface->string_id, interface_name) == 0) {
        eluk_led_interfaces.wmi = NULL;
    } else {
        mutex_unlock(&eluk_led_interface_modification_lock);
        return -EINVAL;
    }

    mutex_unlock(&eluk_led_interface_modification_lock);

    return 0;
}
EXPORT_SYMBOL(eluk_led_remove_interface);

u32 eluk_led_get_active_interface_id(char **id_str)
{
    if (IS_ERR_OR_NULL(eluk_led_interfaces.wmi))
        return -ENODEV;

    if (!IS_ERR_OR_NULL(id_str))
        *id_str = eluk_led_interfaces.wmi->string_id;

    return 0;
}
EXPORT_SYMBOL(eluk_led_get_active_interface_id);

void eluk_led_evt_cb_int(u32 code)
{
    // NOOP
}

#if !defined(ELUK_BUF_LOGGING)
void eluk_led_evt_cb_buf(u8 b_l, u8* b_ptr)
{
    // NOOP
}
#endif
#endif