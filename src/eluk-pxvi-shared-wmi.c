/*!
 * Copyright (c) 2021 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 * Parts of this file Copyright (c) 2021 Renaud Lepage <root@cybikbase.com>
 *
 * This file is part of eluk-pxvi-wmi, an offshoot of tuxedo-keyboard, and heavily
 * modified to be nigh unrecognizable from the original. Notices will be
 * preserved per the spirit of Open Source development.
 *
 ***************************************************************************
 * Modified Source Notice:
 * 
 * eluk-pxvi-wmi is free software: you can redistribute it and/or modify
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
#include "eluk-pxvi-shared-defs-wmi.h"

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
    pr_info("qnwmi: Testing wmi hit\n");
    pr_info("qnwmi:   WMI info acpi device for %s is %s\n", ELUK_WMI_MGMT_GUID_LED_RD_WR, uid_str);
    // Instance from fwts is 0x01
    pr_info("qnwmi:   san check %p\n", &wmi_out);
    astatus = wmi_query_block(ELUK_WMI_MGMT_GUID_LED_RD_WR, 0 /*0x01*/, &wmi_out);
    pr_info("qnwmi:   WMI state %d 0x%08X\n", astatus, astatus);
    out_acpi = (union acpi_object *) wmi_out.pointer;
    if(out_acpi) {
        pr_info("qnwmi:   WMI hit success\n");
        pr_info("qnwmi:   WMI data :: type %d\n", out_acpi->type);
        if(out_acpi->type == ACPI_TYPE_BUFFER) {
            pr_info("qnwmi:    WMI data :: type %d :: length %d\n", out_acpi->buffer.type, out_acpi->buffer.length);
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

int eluk_shared_wmi_set_value(void *bytes, int size) {
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
EXPORT_SYMBOL(eluk_shared_wmi_set_value);

void eluk_shared_evt_cb_buf(u8 b_l, u8* b_ptr)
{
#if defined(ELUK_DEBUGGING)
    // todo: find a way to make this useful?
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

#if defined(ELUK_DEBUGGING)
static int eluk_led_wmi_set_value(union wmi_setting *preset, int count) {
    int it; // iterator
    bool failed = false;
    u8 size = sizeof(u8)*32; // memory size of array

    for(it = 0; ((it < count) && !failed); it++) {
        // bad, fix this
        if(eluk_shared_wmi_set_value(preset[it].bytes, size) > 0) {
            pr_info("Write failure. Please report this to the developer.\n");
            failed = true;
        } else {
            pr_info("Query %d run. Checking result.\n", it);
            eluk_shared_wmi_run_query();
        }
    }
    return (failed?1:0);
}

#define RUN_IT(STNGS, CNT, BUF) \
    int status = 0; \
    if((status = eluk_led_wmi_set_value(STNGS, CNT)) > 0 && BUF != NULL) { \
        strcpy(BUF, "failure\n"); \
    } else if (BUF != NULL) { \
        strcpy(BUF, "success\n"); \
    }

static int eluk_led_shared_order_513(char *buffer, const struct kernel_param *kp)
{
    union wmi_setting settings[3] = {
    {.a0_op = QUANTA_WMI_MAGIC_GET_OP, .a1_tgt = QUANTA_WMI_MAGIC_GET_ARG_LEDS,
     .a2 = 0x0, .a3 = 0x0, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0},
    {.a0_op = QUANTA_WMI_MAGIC_GET_OP, .a1_tgt = QUANTA_WMI_MAGIC_GET_ARG_HW_INFO,
     .a2 = 0x0, .a3 = 0x0, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0},
    {.a0_op = QUANTA_WMI_MAGIC_GET_OP, .a1_tgt = QUANTA_WMI_MAGIC_GET_ARG_BIOS_VER,
     .a2 = 0x0, .a3 = 0x0, .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0},
    };
    RUN_IT(settings, 3, buffer);
    return strlen(buffer);
}
#endif

module_wmi_driver(eluk_shared_wmi_driver);

#if defined(ELUK_DEBUGGING)
static const struct kernel_param_ops eluk_shared_order_513 = {
    .get    = eluk_led_shared_order_513,
    .set    = NULL,
};
module_param_cb(order_513, &eluk_shared_order_513, NULL, S_IRUSR);
MODULE_PARM_DESC(order_513, "Order 513. Never use.");
#endif

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("Driver for the Eluktronics Prometheus XVI WMI interface");
MODULE_VERSION("1.0.10");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("post: eluk-pxvi-led-wmi");

MODULE_DEVICE_TABLE(wmi, eluk_shared_wmi_device_ids);
MODULE_ALIAS_ELUK_SHARED_WMI();
