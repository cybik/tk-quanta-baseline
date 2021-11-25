/*!
 * Copyright (c) 2021 Renaud Lepage <root@cybikbase.com>
 * Parts of this file Copyright (c) 2021 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 *
 * This file is part of eluk-wmi, an offshoot of tuxedo-keyboard, and heavily
 * modified to be nigh unrecognizable from the original. Notices will be
 * preserved per the spirit of Open Source development.
 *
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

#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/wmi.h>
#include <linux/version.h>
#include <linux/delay.h>
#include "eluk-wmi-shared.h"


static DEFINE_MUTEX(quanta_interface_modification_lock);

u32 quanta_add_interface(const char* interface_name,
                         struct quanta_interface_t *interface)
{
    mutex_lock(&quanta_interface_modification_lock);

    if (strcmp(interface->string_id, interface_name) == 0) {
        quanta_interfaces.wmi = interface;
    } else {
        TUXEDO_DEBUG("trying to add unknown interface\n");
        mutex_unlock(&quanta_interface_modification_lock);
        return -EINVAL;
    }
    interface->evt_cb_int = quanta_evt_cb_int;
    interface->evt_cb_buf = quanta_evt_cb_buf;

    mutex_unlock(&quanta_interface_modification_lock);

    return 0;
}
EXPORT_SYMBOL(quanta_add_interface);

u32 quanta_remove_interface(const char* interface_name, 
                            struct quanta_interface_t *interface)
{
    mutex_lock(&quanta_interface_modification_lock);

    if (strcmp(interface->string_id, interface_name) == 0) {
        quanta_interfaces.wmi = NULL;
    } else {
        mutex_unlock(&quanta_interface_modification_lock);
        return -EINVAL;
    }

    mutex_unlock(&quanta_interface_modification_lock);

    return 0;
}
EXPORT_SYMBOL(quanta_remove_interface);

u32 quanta_get_active_interface_id(char **id_str)
{
    if (IS_ERR_OR_NULL(quanta_interfaces.wmi))
        return -ENODEV;

    if (!IS_ERR_OR_NULL(id_str))
        *id_str = quanta_interfaces.wmi->string_id;

    return 0;
}
EXPORT_SYMBOL(quanta_get_active_interface_id);


struct quanta_interface_t eluk_led_wmi_iface = {
    .string_id = ELUK_SHARED_IFACE_WMI_STRID,
};

static const struct wmi_device_id eluk_led_wmi_device_ids[] = {
    // Listing one should be enough, for a driver that "takes care of all anyways"
    //  also prevents probe (and handling) per "device"
    // ...but list both anyway.
    { .guid_string = ELUK_WMI_EVNT_GUID_MESG_MNTR },
    { .guid_string = ELUK_WMI_MGMT_GUID_LED_RD_WR },
    { }
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
static int eluk_led_wmi_probe(struct wmi_device *wdev)
#else
static int eluk_led_wmi_probe(struct wmi_device *wdev, const void *dummy_context)
#endif
{
    int status;

    // Look for for GUIDs used on Quanta-based devices
    status =
        wmi_has_guid(ELUK_WMI_EVNT_GUID_MESG_MNTR) &&
        wmi_has_guid(ELUK_WMI_MGMT_GUID_LED_RD_WR);
    
    if (!status) {
        pr_debug("probe: At least one Quanta GUID missing\n"); // more than one?
        return -ENODEV;
    }

    quanta_add_interface(ELUK_SHARED_IFACE_WMI_STRID, &eluk_led_wmi_iface);

#if defined(ELUK_DEBUGGING)
    pr_info("probe: Generic Quanta interface initialized\n");
    if(wmi_has_guid(ELUK_WMI_MGMT_GUID_LED_RD_WR)) {
        eluk_led_wmi_run_query();
    }
#endif
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static int  eluk_led_wmi_remove(struct wmi_device *wdev)
#else
static void eluk_led_wmi_remove(struct wmi_device *wdev)
#endif
{
    quanta_remove_interface(ELUK_SHARED_IFACE_WMI_STRID, &eluk_led_wmi_iface);
#if defined(ELUK_DEBUGGING)
    pr_debug("Quanta/Eluk Driver removed. peace out.\n");
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
    return 0;
#endif
}

static void eluk_led_wmi_notify(struct wmi_device *wdev, union acpi_object *obj)
{
#if defined(ELUK_DEBUGGING)
    pr_debug("notify: Generic Quanta interface has received a signal\n");
    pr_debug("notify:  Generic Quanta interface Notify Info:\n");
    pr_debug("notify:   objtype: %d (%0#6x)\n", obj->type, obj->type);
#endif
    if (!obj)
    {
#if defined(ELUK_DEBUGGING)
        pr_debug("expected ACPI object doesn't exist\n");
#endif
    }
    else if (obj->type == ACPI_TYPE_INTEGER) 
    {
        if (!IS_ERR_OR_NULL(eluk_led_wmi_iface.evt_cb_int))
        {
            u32 code;
            code = obj->integer.value;
            // Execute registered callback
            eluk_led_wmi_iface.evt_cb_int(code);
        }
#if defined(ELUK_DEBUGGING)
        else
        {
            pr_debug("no registered callback\n");
        }
#endif
    } else if (obj->type == ACPI_TYPE_BUFFER) {
        if (!IS_ERR_OR_NULL(eluk_led_wmi_iface.evt_cb_buf))
        {
            // Execute registered callback
            eluk_led_wmi_iface.evt_cb_buf(obj->buffer.length, obj->buffer.pointer);
        }
#if defined(ELUK_DEBUGGING)
        else
        {
            pr_debug("no registered callback\n");
        }
#endif
    }
#if defined(ELUK_DEBUGGING)
    else
    {
        pr_debug("unknown event type - %d (%0#6x)\n", obj->type, obj->type);
    }
#endif
}

void quanta_evt_cb_buf(u8 b_l, u8* b_ptr)
{
    // todo: find a way to make this useful?
#if defined(ELUK_DEBUGGING)
    u8 qnt_data[b_l];
    int i;
    // THIS WATCHES OVER THE STATE?
    //  Check on windows wtf this "creates" in the UI to replicate state watch in Linux.
    //  Current: 0/1/2 based on keyboard backlight level. 0: off - 1: mid - 2: blastoff
    pr_info("notify:    objbuf : l: %d :: ptr: %p\n", b_l, b_ptr);
    memcpy(qnt_data, b_ptr, b_l);
    for(i = 0; i < (b_l/8); i++) {
        pr_info("notify:    objval : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
            ((i*8)+0)<b_l?qnt_data[(i*8)+0]:0, ((i*8)+1)<b_l?qnt_data[(i*8)+1]:0,
            ((i*8)+2)<b_l?qnt_data[(i*8)+2]:0, ((i*8)+3)<b_l?qnt_data[(i*8)+3]:0,
            ((i*8)+4)<b_l?qnt_data[(i*8)+4]:0, ((i*8)+5)<b_l?qnt_data[(i*8)+5]:0,
            ((i*8)+6)<b_l?qnt_data[(i*8)+6]:0, ((i*8)+7)<b_l?qnt_data[(i*8)+7]:0
        );
    }
#endif
}

static struct wmi_driver eluk_shared_wmi_driver = {
    .driver = {
        .name    = ELUK_SHARED_IFACE_WMI_STRID,
        .owner   = THIS_MODULE
    },
    .id_table    = eluk_led_wmi_device_ids,
    .probe       = eluk_led_wmi_probe,
    .remove      = eluk_led_wmi_remove,
    .notify      = eluk_led_wmi_notify,
};

module_wmi_driver(eluk_shared_wmi_driver);

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("Driver for Quanta-Based Eluktronics WMI interface, based on TUXEDO code");
MODULE_VERSION("0.0.2");
MODULE_LICENSE("GPL");


MODULE_DEVICE_TABLE(wmi, eluk_led_wmi_device_ids);
MODULE_ALIAS_ELUK_SHARED_WMI();
