/*!
 * Copyright (c) 2021 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 * Parts of this file Copyright (c) 2021 Renaud Lepage <root@cybikbase.com>
 *
 * This file is originally from the tuxedo-keyboard project, and heavily
 * modified to be nigh unrecognizable from the original. Notices will be
 * preserved per the spirit of Open Source development.
 *
 ***************************************************************************
 * Modified Source Notice:
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
#include "eluk-led-wmi.h"


// Module-wide values for setting. Has "original" unset default values

// Default Color Settings
static uint eluk_kbd_rgb_set_logo_color  = 0x00FFFF; // Default: Nani
static uint eluk_kbd_rgb_set_trunk_color = 0x00FFFF; // Default: Nani
static uint eluk_kbd_rgb_set_left_color  = 0xFF0000; // Default: Red
static uint eluk_kbd_rgb_set_cntr_color  = 0x00FF00; // Default: Green
static uint eluk_kbd_rgb_set_right_color = 0x0000FF; // Default: Blue

// Default Effect / Intensity Settings. use << 5? - issue is the base storage is too big argh
static uint eluk_kbd_rgb_set_logo_alpha  = 0x10; // Default: Online? to check
static uint eluk_kbd_rgb_set_trunk_alpha = 0x10; // Default: Online? to check
static uint eluk_kbd_rgb_set_left_alpha  = 0x12; // Default: 100%
static uint eluk_kbd_rgb_set_cntr_alpha  = 0x12; // Default: 100%
static uint eluk_kbd_rgb_set_right_alpha = 0x12; // Default: 100%

// Commit predefs
static int eluk_led_wmi_colors_commit_all(const char *val, const struct kernel_param *kp);

DEFINE_MUTEX(eluk_wmi_lock); // unused?

struct quanta_interface_t eluk_led_wmi_iface = {
    .string_id = ELUK_LED_IFACE_WMI_STRID,
};

static void eluk_led_wmi_run_query(void)
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
#if 0
            quanta_evt_cb_buf(out_acpi->buffer.length, out_acpi->buffer.pointer);
#endif
        }
        kfree(out_acpi);
    } else {
        pr_info("qnwmi:   WMI hit failure\n");
    }
}

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

    quanta_add_interface(ELUK_LED_IFACE_WMI_STRID, &eluk_led_wmi_iface);

    pr_info("probe: Generic Quanta interface initialized\n");

    if(wmi_has_guid(ELUK_WMI_MGMT_GUID_LED_RD_WR)) {
        eluk_led_wmi_run_query();
    }
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static int  eluk_led_wmi_remove(struct wmi_device *wdev)
#else
static void eluk_led_wmi_remove(struct wmi_device *wdev)
#endif
{
    pr_info("Driver removed. peace out.\n");
    quanta_remove_interface(ELUK_LED_IFACE_WMI_STRID, &eluk_led_wmi_iface);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
    return 0;
#endif
}

static void eluk_led_wmi_notify(struct wmi_device *wdev, union acpi_object *obj)
{
    pr_info("notify: Generic Quanta interface has received a signal\n");
    pr_info("notify:  Generic Quanta interface Notify Info:\n");
    pr_info("notify:   objtype: %d (%0#6x)\n", obj->type, obj->type);
    if (!obj) {
        pr_debug("expected ACPI object doesn't exist\n");
    } else if (obj->type == ACPI_TYPE_INTEGER) {
        if (!IS_ERR_OR_NULL(eluk_led_wmi_iface.evt_cb_int)) {
            u32 code;
            code = obj->integer.value;
            // Execute registered callback
            eluk_led_wmi_iface.evt_cb_int(code);
        } else {
            pr_debug("no registered callback\n");
        }
    } else if (obj->type == ACPI_TYPE_BUFFER) {
        if (!IS_ERR_OR_NULL(eluk_led_wmi_iface.evt_cb_buf)) {
            // Execute registered callback
            eluk_led_wmi_iface.evt_cb_buf(obj->buffer.length, obj->buffer.pointer);
        } else {
            pr_debug("no registered callback\n");
        }
    } else {
        pr_debug("unknown event type - %d (%0#6x)\n", obj->type, obj->type);
    }
}

void quanta_evt_cb_buf(u8 b_l, u8* b_ptr)
{
    // todo: find a way to make this useful?
#if 1
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

static const struct wmi_device_id eluk_led_wmi_device_ids[] = {
    // Listing one should be enough, for a driver that "takes care of all anyways"
    //  also prevents probe (and handling) per "device"
    // ...but list both anyway.
    { .guid_string = ELUK_WMI_EVNT_GUID_MESG_MNTR },
    { .guid_string = ELUK_WMI_MGMT_GUID_LED_RD_WR },
    { }
};

static struct wmi_driver eluk_led_wmi_driver = {
    .driver = {
        .name    = ELUK_LED_IFACE_WMI_STRID,
        .owner   = THIS_MODULE
    },
    .id_table    = eluk_led_wmi_device_ids,
    .probe       = eluk_led_wmi_probe,
    .remove      = eluk_led_wmi_remove,
    .notify      = eluk_led_wmi_notify,
};

static int eluk_led_wmi_set_value_exec(union wmi_setting *preset, int count) {
    struct acpi_buffer input;
    acpi_status status;
    int iter;
    bool failed = false;

    for(iter = 0; ((iter < count) && !failed); iter++) {
        pr_debug("Attempting to set LED colors via WMI.\n");

        input.length = (sizeof(u8)*32); // u8 array
        input.pointer = preset[iter].bytes;
        status = wmi_set_block(ELUK_WMI_MGMT_GUID_LED_RD_WR, 0, &input);
        if (ACPI_FAILURE(status)) {
            pr_info("Write fail, meh. We're debugging at this point.\n");
            pr_info("Reboot to windows to fix the state?\n");
            failed = true;
        }
        // No need to read after; this module only sets and doesn't query.
    }
    return (failed?1:0);
}


static void eluk_led_wmi_set_default_colors(void)
{
    eluk_kbd_rgb_set_logo_color  = 0x00FFFF;
    eluk_kbd_rgb_set_trunk_color = 0x00FFFF;
    eluk_kbd_rgb_set_left_color  = 0xFF0000;
    eluk_kbd_rgb_set_cntr_color  = 0x00FF00;
    eluk_kbd_rgb_set_right_color = 0x0000FF;
}

static void eluk_led_wmi_set_kbd_zones_effect(u8 val)
{
    eluk_kbd_rgb_set_left_alpha  = val;
    eluk_kbd_rgb_set_cntr_alpha  = val;
    eluk_kbd_rgb_set_right_alpha = val;
}

static int eluk_led_wmi_rgb_offline(const char *val, const struct kernel_param *kp)
{
    return eluk_led_wmi_set_value_exec(offline_union, 5);
}

static int eluk_led_wmi_rgb_solid_50(const char *val, const struct kernel_param *kp)
{
    eluk_led_wmi_set_default_colors();
    eluk_led_wmi_set_kbd_zones_effect(0x11);
    eluk_kbd_rgb_set_logo_alpha  = 0x10;
    eluk_kbd_rgb_set_trunk_alpha = 0x10;
    eluk_led_wmi_colors_commit_all(NULL, NULL);
    return eluk_led_wmi_set_value_exec(solid_50_union, 5);
}

static int eluk_led_wmi_rgb_solid_100(const char *val, const struct kernel_param *kp)
{
    return eluk_led_wmi_set_value_exec(solid_100_union, 5);
}

static int eluk_led_wmi_rgb_breathing_50(const char *val, const struct kernel_param *kp)
{
    return eluk_led_wmi_set_value_exec(breathing_50_union, 5);
}

static int eluk_led_wmi_rgb_breathing_100(const char *val, const struct kernel_param *kp)
{
    return eluk_led_wmi_set_value_exec(breathing_100_union, 5);
}

static int eluk_led_wmi_colors_commit_all(const char *val, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting create_struct[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // trunk/logo?
        .a2 = ELUK_WMI_LED_ZONE_LOGO,    .a3 = ((eluk_kbd_rgb_set_logo_alpha  << 24) + eluk_kbd_rgb_set_logo_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // logo/trunk?
        .a2 = ELUK_WMI_LED_ZONE_TRUNK,   .a3 = ((eluk_kbd_rgb_set_trunk_alpha << 24) + eluk_kbd_rgb_set_trunk_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led3 - right
        .a2 = ELUK_WMI_LED_ZONE_RIGHT,   .a3 = ((eluk_kbd_rgb_set_right_alpha  << 24) + eluk_kbd_rgb_set_right_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led2 - centre
        .a2 = ELUK_WMI_LED_ZONE_CENTRE,  .a3 = ((eluk_kbd_rgb_set_cntr_alpha  << 24) + eluk_kbd_rgb_set_cntr_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led1 - left
        .a2 = ELUK_WMI_LED_ZONE_LEFT,    .a3 = ((eluk_kbd_rgb_set_left_alpha << 24) + eluk_kbd_rgb_set_left_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, 
    };

    if(val != NULL && val[0] == 'a')
    {
        pr_info("Fake commit! Verify the created:\n");
        quanta_evt_cb_buf(32, create_struct->bytes);
        pr_info("Fake commit! Verify against the original:\n");
        quanta_evt_cb_buf(32, solid_50_union->bytes);
        return 0;
    }
    
    return eluk_led_wmi_set_value_exec(create_struct, 5);
}

static int eluk_led_wmi_colors_commit_kbd(const char *val, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting create_struct[3] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led3 - right
        .a2 = ELUK_WMI_LED_ZONE_RIGHT,   .a3 = ((eluk_kbd_rgb_set_right_alpha  << 24) + eluk_kbd_rgb_set_right_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led2 - centre
        .a2 = ELUK_WMI_LED_ZONE_CENTRE,  .a3 = ((eluk_kbd_rgb_set_cntr_alpha  << 24) + eluk_kbd_rgb_set_cntr_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led1 - left
        .a2 = ELUK_WMI_LED_ZONE_LEFT,    .a3 = ((eluk_kbd_rgb_set_left_alpha << 24) + eluk_kbd_rgb_set_left_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    };
    return eluk_led_wmi_set_value_exec(create_struct, 3);
}

static int eluk_led_wmi_colors_commit_trunk(const char *val, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting create_struct[1] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // logo/trunk?
        .a2 = ELUK_WMI_LED_ZONE_TRUNK,   .a3 = ((eluk_kbd_rgb_set_trunk_alpha << 24) + eluk_kbd_rgb_set_trunk_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }
    };
    return eluk_led_wmi_set_value_exec(create_struct, 1);
}

static int eluk_led_wmi_colors_commit_logo(const char *val, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting create_struct[1] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // trunk/logo?
        .a2 = ELUK_WMI_LED_ZONE_LOGO,    .a3 = ((eluk_kbd_rgb_set_logo_alpha  << 24) + eluk_kbd_rgb_set_logo_color),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }
    };
    return eluk_led_wmi_set_value_exec(create_struct, 1);
}

module_wmi_driver(eluk_led_wmi_driver);

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("Driver for Quanta-Based Eluktronics WMI interface, based on TUXEDO code");
MODULE_VERSION("0.0.2");
MODULE_LICENSE("GPL");

// section: preset ops
static const struct kernel_param_ops eluk_kbd_preset_offline_ops = {
    .set    = eluk_led_wmi_rgb_offline,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_offline, &eluk_kbd_preset_offline_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_preset_offline, "Apply 0-out RGB driver preset.");

static const struct kernel_param_ops eluk_kbd_preset_solid_50_ops = {
    .set    = eluk_led_wmi_rgb_solid_50,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_solid_50, &eluk_kbd_preset_solid_50_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_preset_solid_50, "Apply Solid Half Brightness RGB driver preset.");
 
static const struct kernel_param_ops eluk_kbd_preset_solid_100_ops = {
    .set    = eluk_led_wmi_rgb_solid_100,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_solid_100, &eluk_kbd_preset_solid_100_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_preset_solid_100, "Apply Solid Full Brightness RGB driver preset.");

static const struct kernel_param_ops eluk_kbd_preset_breathing_50_ops = {
    .set    = eluk_led_wmi_rgb_breathing_50,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_breathing_50, &eluk_kbd_preset_breathing_50_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_preset_breathing_50, "Apply Breathing Half Brightness RGB driver preset.");
 
static const struct kernel_param_ops eluk_kbd_preset_breathing_100_ops = {
    .set    = eluk_led_wmi_rgb_breathing_100,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_breathing_100, &eluk_kbd_preset_breathing_100_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_preset_breathing_100, "Apply Breathing Full Brightness RGB driver preset.");
// endsection: preset ops

// section: Zone Colors
module_param(eluk_kbd_rgb_set_logo_color, uint, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_logo_color, "Color for the Logo.");

module_param(eluk_kbd_rgb_set_trunk_color, uint, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_trunk_color, "Color for the Trunk.");

module_param(eluk_kbd_rgb_set_left_color, uint, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_left_color, "Color for the Left.");

module_param(eluk_kbd_rgb_set_cntr_color, uint, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_cntr_color, "Color for the Center.");

module_param(eluk_kbd_rgb_set_right_color, uint, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_right_color, "Color for the Right.");
// endsection: Zone Colors


// section: Effect/Brightness Setting
// TODO: can these be made smaller my lord.
module_param(eluk_kbd_rgb_set_logo_alpha, int, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_logo_alpha, "Effect / Brightness for the Logo.");

module_param(eluk_kbd_rgb_set_trunk_alpha, int, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_trunk_alpha, "Effect / Brightness for the Trunk.");

module_param(eluk_kbd_rgb_set_left_alpha, int, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_left_alpha, "Effect / Brightness for the Left.");

module_param(eluk_kbd_rgb_set_cntr_alpha, int, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_cntr_alpha, "Effect / Brightness for the Center.");

module_param(eluk_kbd_rgb_set_right_alpha, int, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_set_right_alpha, "Effect / Brightness for the Right.");
// endsection: Effect/Brightness Setting


// section: commit ops
static const struct kernel_param_ops eluk_commit_all_ops = {
    .set    = eluk_led_wmi_colors_commit_all,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_all, &eluk_commit_all_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_commit_all, "Commit all colors and mode setup to WMI.");

static const struct kernel_param_ops eluk_commit_kbd_ops = {
    .set    = eluk_led_wmi_colors_commit_kbd,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_kbd, &eluk_commit_kbd_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_commit_kbd, "Commit keyboard colors and mode setup to WMI.");

static const struct kernel_param_ops eluk_commit_trunk_ops = {
    .set    = eluk_led_wmi_colors_commit_trunk,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_trunk, &eluk_commit_trunk_ops, NULL, S_IWUSR | S_IWGRP );
MODULE_PARM_DESC(eluk_kbd_rgb_commit_trunk, "Commit trunk colors and mode setup to WMI.");

// Unused on Eluktronics
static const struct kernel_param_ops eluk_commit_logo_ops = {
    .set    = eluk_led_wmi_colors_commit_logo,
    .get    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_logo, &eluk_commit_logo_ops, NULL, S_IWUSR | S_IWGRP  );
MODULE_PARM_DESC(eluk_kbd_rgb_commit_logo, "Commit logo colors and mode setup to WMI.");
// endsection: commit ops

MODULE_DEVICE_TABLE(wmi, eluk_led_wmi_device_ids);
MODULE_ALIAS_ELUK_LED_WMI();
