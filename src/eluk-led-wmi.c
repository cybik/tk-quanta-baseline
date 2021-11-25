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
#include "eluk-led-wmi.h"
#include "eluk-shared-wmi.h"

#define ELUK_EXPERIMENTAL

// Bitwise macro to create the color.
#define BITWISE_A3(X, Y, Z) ((((X << 4) | Y) << 24) | Z)

// Module-wide values for setting. Has "original" unset default values

// section: Defaults

/**********************************************************
 *
 * These default values imitate original vendor settings.
 *
 **********************************************************/

// Default Colors
static uint eluk_kbd_rgb_set_logo_color   = 0x00FFFF; // Default: Teal
static uint eluk_kbd_rgb_set_trunk_color  = 0x00FFFF; // Default: Teal
static uint eluk_kbd_rgb_set_left_color   = 0xFF0000; // Default: Red
static uint eluk_kbd_rgb_set_cntr_color   = 0x00FF00; // Default: Green
static uint eluk_kbd_rgb_set_right_color  = 0x0000FF; // Default: Blue

// Default Effect    ( << 28 )
static uint eluk_kbd_rgb_set_logo_effect  = 0x1;      // Default: Online? to check
static uint eluk_kbd_rgb_set_trunk_effect = 0x1;      // Default: Online? to check
static uint eluk_kbd_rgb_set_left_effect  = 0x1;      // Default: 100%
static uint eluk_kbd_rgb_set_cntr_effect  = 0x1;      // Default: 100%
static uint eluk_kbd_rgb_set_right_effect = 0x1;      // Default: 100%

// Default Intensity ( << 24 )
static uint eluk_kbd_rgb_set_logo_level  = 0x0;       // Default: Online? to check
static uint eluk_kbd_rgb_set_trunk_level = 0x1;       // Default: 50%
static uint eluk_kbd_rgb_set_left_level  = 0x1;       // Default: 50%
static uint eluk_kbd_rgb_set_cntr_level  = 0x1;       // Default: 50%
static uint eluk_kbd_rgb_set_right_level = 0x1;       // Default: 50%

// endsection: Defaults


// Commit predefs
static int eluk_led_wmi_colors_commit_all(char *, const struct kernel_param *);

//DEFINE_MUTEX(eluk_wmi_lock); // unused?

struct eluk_led_interface_t eluk_led_wmi_iface = {
    .string_id = ELUK_LED_IFACE_WMI_STRID,
};

#if defined(ELUK_DEBUGGING)
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
            eluk_led_evt_cb_buf(out_acpi->buffer.length, out_acpi->buffer.pointer);
        }
        kfree(out_acpi);
    } else {
        pr_info("qnwmi:   WMI hit failure\n");
    }
}
#endif

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

    eluk_led_add_interface(ELUK_LED_IFACE_WMI_STRID, &eluk_led_wmi_iface);

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
    eluk_led_remove_interface(ELUK_LED_IFACE_WMI_STRID, &eluk_led_wmi_iface);
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

#if defined(ELUK_DEBUGGING)
void eluk_led_evt_cb_buf(u8 b_l, u8* b_ptr)
{
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
}
#endif

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
    int iter;
    bool failed = false;

    for(iter = 0; ((iter < count) && !failed); iter++) {
        // bad, fix this
        if(eluk_shared_wmi_set_value_exec(preset[iter].bytes, (sizeof(u8)*32)) > 0) {
            pr_info("Write failure. Please report this to the developer.\n");
            failed = true;
        }
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
    eluk_kbd_rgb_set_left_effect  = val;
    eluk_kbd_rgb_set_cntr_effect  = val;
    eluk_kbd_rgb_set_right_effect = val;
}

static void eluk_led_wmi_set_kbd_zones_level(u8 val)
{
    eluk_kbd_rgb_set_left_level   = val;
    eluk_kbd_rgb_set_cntr_level   = val;
    eluk_kbd_rgb_set_right_level  = val;
}

static int eluk_led_wmi_rgb_offline(char *buffer, const struct kernel_param *kp)
{
    // Set offline, but hard for now.
    eluk_led_wmi_set_default_colors();
    eluk_led_wmi_set_kbd_zones_effect (0x1);
    eluk_led_wmi_set_kbd_zones_level  (0x0);
    eluk_kbd_rgb_set_logo_effect     = 0x1;
    eluk_kbd_rgb_set_logo_level      = 0x0;
    eluk_kbd_rgb_set_trunk_effect    = 0x1;
    eluk_kbd_rgb_set_trunk_level     = 0x0;
    strcpy(buffer, (eluk_led_wmi_colors_commit_all(NULL, NULL)>0?"1\n":"0\n"));
    return strlen(buffer);
}

static int eluk_led_wmi_rgb_solid_50(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_set_default_colors();
    eluk_led_wmi_set_kbd_zones_effect (0x1);
    eluk_led_wmi_set_kbd_zones_level  (0x1);
    eluk_kbd_rgb_set_logo_effect     = 0x1;
    eluk_kbd_rgb_set_logo_level      = 0x0;
    eluk_kbd_rgb_set_trunk_effect    = 0x1;
    eluk_kbd_rgb_set_trunk_level     = 0x1;
    strcpy(buffer, (eluk_led_wmi_colors_commit_all(NULL, NULL)>0?"1\n":"0\n"));
    return strlen(buffer);
}

static int eluk_led_wmi_rgb_solid_100(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_set_default_colors();
    eluk_led_wmi_set_kbd_zones_effect (0x1);
    eluk_led_wmi_set_kbd_zones_level  (0x2);
    eluk_kbd_rgb_set_logo_effect     = 0x1;
    eluk_kbd_rgb_set_logo_level      = 0x0;
    eluk_kbd_rgb_set_trunk_effect    = 0x1;
    eluk_kbd_rgb_set_trunk_level     = 0x2;
    strcpy(buffer, (eluk_led_wmi_colors_commit_all(NULL, NULL)>0?"1\n":"0\n"));
    return strlen(buffer);
}

static int eluk_led_wmi_rgb_breathing_50(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_set_default_colors();
    eluk_led_wmi_set_kbd_zones_effect (0x3);
    eluk_led_wmi_set_kbd_zones_level  (0x1);
    eluk_kbd_rgb_set_logo_effect     = 0x1;
    eluk_kbd_rgb_set_logo_level      = 0x0;
    eluk_kbd_rgb_set_trunk_effect    = 0x3;
    eluk_kbd_rgb_set_trunk_level     = 0x1;
    strcpy(buffer, (eluk_led_wmi_colors_commit_all(NULL, NULL)>0?"1\n":"0\n"));
    return strlen(buffer);
}

static int eluk_led_wmi_rgb_breathing_100(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_set_default_colors();
    eluk_led_wmi_set_kbd_zones_effect (0x3);
    eluk_led_wmi_set_kbd_zones_level  (0x2);
    eluk_kbd_rgb_set_logo_effect     = 0x1;
    eluk_kbd_rgb_set_logo_level      = 0x0;
    eluk_kbd_rgb_set_trunk_effect    = 0x3;
    eluk_kbd_rgb_set_trunk_level     = 0x2;
    strcpy(buffer, (eluk_led_wmi_colors_commit_all(NULL, NULL)>0?"1\n":"0\n"));
    return strlen(buffer);
}

static int eluk_led_wmi_get_logo_a3(void)
{
    return BITWISE_A3(eluk_kbd_rgb_set_logo_effect, eluk_kbd_rgb_set_logo_level, eluk_kbd_rgb_set_logo_color);
}
static int eluk_led_wmi_get_trunk_a3(void)
{
    return BITWISE_A3(eluk_kbd_rgb_set_trunk_effect, eluk_kbd_rgb_set_trunk_level, eluk_kbd_rgb_set_trunk_color);
}
static int eluk_led_wmi_get_right_a3(void)
{
    return BITWISE_A3(eluk_kbd_rgb_set_right_effect, eluk_kbd_rgb_set_right_level, eluk_kbd_rgb_set_right_color);
}
static int eluk_led_wmi_get_centre_a3(void)
{
    return BITWISE_A3(eluk_kbd_rgb_set_cntr_effect, eluk_kbd_rgb_set_cntr_level, eluk_kbd_rgb_set_cntr_color);
}
static int eluk_led_wmi_get_left_a3(void)
{
    return BITWISE_A3(eluk_kbd_rgb_set_left_effect, eluk_kbd_rgb_set_left_level, eluk_kbd_rgb_set_left_color);
}

#define APPLY_SETTINGS(SETTINGS, COUNT, BUFFER) \
    int status = 0; \
    if((status = eluk_led_wmi_set_value_exec(SETTINGS, COUNT)) > 0 && BUFFER != NULL) { \
        strcpy(BUFFER, "failure\n"); \
    } else if (BUFFER != NULL) { \
        strcpy(BUFFER, "success\n"); \
    } \
    return (BUFFER!=NULL?strlen(BUFFER):status)

static int eluk_led_wmi_colors_commit_all(char *buffer, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // trunk/logo?
        .a2 = ELUK_WMI_LED_ZONE_LOGO,    .a3 = eluk_led_wmi_get_logo_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // logo/trunk?
        .a2 = ELUK_WMI_LED_ZONE_TRUNK,   .a3 = eluk_led_wmi_get_trunk_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led3 - right
        .a2 = ELUK_WMI_LED_ZONE_RIGHT,   .a3 = eluk_led_wmi_get_right_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led2 - centre
        .a2 = ELUK_WMI_LED_ZONE_CENTRE,  .a3 = eluk_led_wmi_get_centre_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led1 - left
        .a2 = ELUK_WMI_LED_ZONE_LEFT,    .a3 = eluk_led_wmi_get_left_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }, 
    };

    // Run it.
    APPLY_SETTINGS(settings, 5, buffer); // returns
}

static int eluk_led_wmi_colors_commit_kbd(char *buffer, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[3] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led3 - right
        .a2 = ELUK_WMI_LED_ZONE_RIGHT,   .a3 = eluk_led_wmi_get_right_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led2 - centre
        .a2 = ELUK_WMI_LED_ZONE_CENTRE,  .a3 = eluk_led_wmi_get_centre_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // led1 - left
        .a2 = ELUK_WMI_LED_ZONE_LEFT,    .a3 = eluk_led_wmi_get_left_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 },
    };

    // Run it.
    APPLY_SETTINGS(settings, 3, buffer); // returns
}

static int eluk_led_wmi_colors_commit_trunk(char *buffer, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[1] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // logo/trunk?
        .a2 = ELUK_WMI_LED_ZONE_TRUNK,   .a3 = eluk_led_wmi_get_trunk_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }
    };

    // Run it.
    APPLY_SETTINGS(settings, 1, buffer); // returns
}

static int eluk_led_wmi_colors_commit_logo(char *buffer, const struct kernel_param *kp)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[1] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP,   .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED, // trunk/logo?
        .a2 = ELUK_WMI_LED_ZONE_LOGO,    .a3 = eluk_led_wmi_get_logo_a3(),
        .a4 = 0x0, .a5 = 0x0, .a6 = 0x0, .rev0 = 0x0, .rev1 = 0x0 }
    };
    // Run it.
    APPLY_SETTINGS(settings, 1, buffer); // returns
}

module_wmi_driver(eluk_led_wmi_driver);

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("Driver for Quanta-Based Eluktronics WMI interface, based on TUXEDO code");
MODULE_VERSION("0.0.4");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: eluk-shared-wmi");

// Readonly perm macros
#define PERM_RO       (S_IRUSR | S_IRGRP | S_IROTH)
#define PERM_RO_ADMIN (S_IRUSR | S_IRGRP)

// section: preset ops
static const struct kernel_param_ops eluk_kbd_preset_offline_ops = {
    .get    = eluk_led_wmi_rgb_offline,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_offline, &eluk_kbd_preset_offline_ops, NULL, PERM_RO);
MODULE_PARM_DESC(eluk_kbd_rgb_preset_offline, "Apply 0-out RGB driver preset.");

static const struct kernel_param_ops eluk_kbd_preset_solid_50_ops = {
    .get    = eluk_led_wmi_rgb_solid_50,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_solid_50, &eluk_kbd_preset_solid_50_ops, NULL, PERM_RO);
MODULE_PARM_DESC(eluk_kbd_rgb_preset_solid_50, "Apply Solid Half Brightness RGB driver preset.");
 
static const struct kernel_param_ops eluk_kbd_preset_solid_100_ops = {
    .get    = eluk_led_wmi_rgb_solid_100,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_solid_100, &eluk_kbd_preset_solid_100_ops, NULL, PERM_RO);
MODULE_PARM_DESC(eluk_kbd_rgb_preset_solid_100, "Apply Solid Full Brightness RGB driver preset.");

static const struct kernel_param_ops eluk_kbd_preset_breathing_50_ops = {
    .get    = eluk_led_wmi_rgb_breathing_50,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_breathing_50, &eluk_kbd_preset_breathing_50_ops, NULL, PERM_RO);
MODULE_PARM_DESC(eluk_kbd_rgb_preset_breathing_50, "Apply Breathing Half Brightness RGB driver preset.");
 
static const struct kernel_param_ops eluk_kbd_preset_breathing_100_ops = {
    .get    = eluk_led_wmi_rgb_breathing_100,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_preset_breathing_100, &eluk_kbd_preset_breathing_100_ops, NULL, PERM_RO);
MODULE_PARM_DESC(eluk_kbd_rgb_preset_breathing_100, "Apply Breathing Full Brightness RGB driver preset.");
// endsection: preset ops

// section: Zone Colors
module_param(eluk_kbd_rgb_set_logo_color, uint, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_logo_color, "Color for the Logo.");

module_param(eluk_kbd_rgb_set_trunk_color, uint, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_trunk_color, "Color for the Trunk.");

module_param(eluk_kbd_rgb_set_left_color, uint, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_left_color, "Color for the Left.");

module_param(eluk_kbd_rgb_set_cntr_color, uint, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_cntr_color, "Color for the Center.");

module_param(eluk_kbd_rgb_set_right_color, uint, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_right_color, "Color for the Right.");
// endsection: Zone Colors


// section: Effect/Brightness Setting
// TODO: can these be made smaller my lord.
module_param(eluk_kbd_rgb_set_logo_effect, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_logo_effect, "Effect for the Logo.");

module_param(eluk_kbd_rgb_set_trunk_effect, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_trunk_effect, "Effect for the Trunk.");

module_param(eluk_kbd_rgb_set_left_effect, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_left_effect, "Effect for the Left.");

module_param(eluk_kbd_rgb_set_cntr_effect, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_cntr_effect, "Effect for the Center.");

module_param(eluk_kbd_rgb_set_right_effect, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_right_effect, "Effect for the Right.");

// --

module_param(eluk_kbd_rgb_set_logo_level, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_logo_level, "Brightness for the Logo.");

module_param(eluk_kbd_rgb_set_trunk_level, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_trunk_level, "Brightness for the Trunk.");

module_param(eluk_kbd_rgb_set_left_level, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_left_level, "Brightness for the Left.");

module_param(eluk_kbd_rgb_set_cntr_level, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_cntr_level, "Brightness for the Center.");

module_param(eluk_kbd_rgb_set_right_level, int, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(eluk_kbd_rgb_set_right_level, "Brightness for the Right.");
// endsection: Effect/Brightness Setting


// section: commit ops
static const struct kernel_param_ops eluk_commit_all_ops = {
    .get    = eluk_led_wmi_colors_commit_all,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_all, &eluk_commit_all_ops, NULL, PERM_RO_ADMIN);
MODULE_PARM_DESC(eluk_kbd_rgb_commit_all, "Commit all colors and mode setup to WMI.");

static const struct kernel_param_ops eluk_commit_kbd_ops = {
    .get    = eluk_led_wmi_colors_commit_kbd,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_kbd, &eluk_commit_kbd_ops, NULL, PERM_RO_ADMIN);
MODULE_PARM_DESC(eluk_kbd_rgb_commit_kbd, "Commit keyboard colors and mode setup to WMI.");

static const struct kernel_param_ops eluk_commit_trunk_ops = {
    .get    = eluk_led_wmi_colors_commit_trunk,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_trunk, &eluk_commit_trunk_ops, NULL, PERM_RO_ADMIN);
MODULE_PARM_DESC(eluk_kbd_rgb_commit_trunk, "Commit trunk colors and mode setup to WMI.");

// Unused on Eluktronics
static const struct kernel_param_ops eluk_commit_logo_ops = {
    .get    = eluk_led_wmi_colors_commit_logo,
    .set    = NULL,
};
module_param_cb(eluk_kbd_rgb_commit_logo, &eluk_commit_logo_ops, NULL, PERM_RO_ADMIN);
MODULE_PARM_DESC(eluk_kbd_rgb_commit_logo, "Commit logo colors and mode setup to WMI.");
// endsection: commit ops

MODULE_DEVICE_TABLE(wmi, eluk_led_wmi_device_ids);
MODULE_ALIAS_ELUK_LED_WMI();
