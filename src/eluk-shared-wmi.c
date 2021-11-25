/*!
 * Copyright (c) 2021 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 * Parts of this file Copyright (c) 2021 Renaud Lepage <root@cybikbase.com>
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
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/wmi.h>
#include <linux/version.h>
#include <linux/delay.h>
#include "eluk-shared-defs-wmi.h"

// Module-wide values for setting. Has "original" unset default values
struct eluk_shared_interface_t eluk_shared_wmi_iface = {
    .string_id = ELUK_SHARED_IFACE_WMI_STRID,
};

#if defined(ELUK_DEBUGGING)
static void eluk_shared_wmi_run_query(void)
{
    acpi_status astatus;
    union acpi_object *out_acpi;
    const char *uid_str = wmi_get_acpi_device_uid(ELUK_WMI_MGMT_GUID_LED_RD_WR);
    struct acpi_buffer wmi_out = { ACPI_ALLOCATE_BUFFER, NULL };
    pr_debug("qnwmi: Testing wmi hit\n");
    pr_debug("qnwmi:   WMI info acpi device for %s is %s\n", ELUK_WMI_MGMT_GUID_LED_RD_WR, uid_str);
    // Instance from fwts is 0x01
    pr_debug("qnwmi:   san check %p\n", &wmi_out);
    astatus = wmi_query_block(ELUK_WMI_MGMT_GUID_LED_RD_WR, 0 /*0x01*/, &wmi_out);
    pr_debug("qnwmi:   WMI state %d 0x%08X\n", astatus, astatus);
    out_acpi = (union acpi_object *) wmi_out.pointer;
    if(out_acpi) {
        pr_debug("qnwmi:   WMI hit success\n");
        pr_debug("qnwmi:   WMI data :: type %d\n", out_acpi->type);
        if(out_acpi->type == ACPI_TYPE_BUFFER) {
            pr_debug("qnwmi:    WMI data :: type %d :: length %d\n", out_acpi->buffer.type, out_acpi->buffer.length);
            eluk_shared_evt_cb_buf(out_acpi->buffer.length, out_acpi->buffer.pointer);
        }
        kfree(out_acpi);
    } else {
        pr_info("qnwmi:   WMI hit failure\n");
    }
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
static int eluk_shared_wmi_probe(struct wmi_device *wdev)
#else
static int eluk_shared_wmi_probe(struct wmi_device *wdev, const void *dummy_context)
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

    eluk_shared_add_interface(ELUK_SHARED_IFACE_WMI_STRID, &eluk_shared_wmi_iface);

#if defined(ELUK_DEBUGGING)
    pr_info("probe: Generic Quanta interface initialized\n");
    if(wmi_has_guid(ELUK_WMI_MGMT_GUID_LED_RD_WR)) {
        eluk_shared_wmi_run_query();
    }
#endif
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static int  eluk_shared_wmi_remove(struct wmi_device *wdev)
#else
static void eluk_shared_wmi_remove(struct wmi_device *wdev)
#endif
{
    eluk_shared_remove_interface(ELUK_SHARED_IFACE_WMI_STRID, &eluk_shared_wmi_iface);
#if defined(ELUK_DEBUGGING)
    pr_debug("Quanta/Eluk Driver removed. peace out.\n");
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
    return 0;
#endif
}

static void eluk_shared_wmi_notify(struct wmi_device *wdev, union acpi_object *obj)
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
        if (!IS_ERR_OR_NULL(eluk_shared_wmi_iface.evt_cb_int))
        {
            u32 code;
            code = obj->integer.value;
            // Execute registered callback
            eluk_shared_wmi_iface.evt_cb_int(code);
        }
#if defined(ELUK_DEBUGGING)
        else
        {
            pr_debug("no registered callback\n");
        }
#endif
    } else if (obj->type == ACPI_TYPE_BUFFER) {
        if (!IS_ERR_OR_NULL(eluk_shared_wmi_iface.evt_cb_buf))
        {
            // Execute registered callback
            eluk_shared_wmi_iface.evt_cb_buf(obj->buffer.length, obj->buffer.pointer);
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


static const struct wmi_device_id eluk_shared_wmi_device_ids[] = {
    // Listing one should be enough, for a driver that "takes care of all anyways"
    //  also prevents probe (and handling) per "device"
    // ...but list both anyway.
    { .guid_string = ELUK_WMI_EVNT_GUID_MESG_MNTR },
    { .guid_string = ELUK_WMI_MGMT_GUID_LED_RD_WR },
    { }
};

static struct wmi_driver eluk_shared_wmi_driver = {
    .driver = {
        .name    = ELUK_SHARED_IFACE_WMI_STRID,
        .owner   = THIS_MODULE
    },
    .id_table    = eluk_shared_wmi_device_ids,
    .probe       = eluk_shared_wmi_probe,
    .remove      = eluk_shared_wmi_remove,
    .notify      = eluk_shared_wmi_notify,
};

int eluk_shared_wmi_set_value_exec(void *bytes, int size) {
    struct acpi_buffer input;
    acpi_status status;

    //pr_info("Writing from shared.\n");

    input.length = size; // u8 array
    input.pointer = bytes;
    status = wmi_set_block(ELUK_WMI_MGMT_GUID_LED_RD_WR, 0, &input);
    if (ACPI_FAILURE(status)) {
        return 1;
    }
    return 0;
}
EXPORT_SYMBOL(eluk_shared_wmi_set_value_exec);

module_wmi_driver(eluk_shared_wmi_driver);

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("Driver for the Eluktronics Prometheus XVI WMI interface");
MODULE_VERSION("0.0.4");
MODULE_LICENSE("GPL");

MODULE_DEVICE_TABLE(wmi, eluk_shared_wmi_device_ids);
MODULE_ALIAS_ELUK_SHARED_WMI();
