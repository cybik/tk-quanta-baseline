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

// TODO: Vendor line - imitate dmi, but have it in the driver
// TODO: Vendor line - imitate dmi, but have it in the driver
// TODO: Vendor line - imitate dmi, but have it in the driver

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/wmi.h>
#include <linux/version.h>
#include <linux/delay.h>
#include "eluk-pxvi-led-wmi.h"
#include "eluk-pxvi-shared-wmi.h"

//#define ELUK_EXPERIMENTAL
#define ELUK_TESTING

// Bitwise macro to create the color.
#define BITW_A3(X, Y, Z) ((((X << 4) | Y) << 24) | Z)

// Module-wide values for setting. Has "original" unset default values

// section: Defaults

/**********************************************************
 *
 * These default values imitate original vendor settings.
 *
 **********************************************************/

// Default Colors
static uint rgb_logo_color   = 0x00FFFF; // Default: Teal
static uint rgb_trunk_color  = 0x00FFFF; // Default: Teal
static uint rgb_left_color   = 0xFF0000; // Default: Red
static uint rgb_cntr_color   = 0x00FF00; // Default: Green
static uint rgb_right_color  = 0x0000FF; // Default: Blue

// Default Effect    ( << 28 )
static uint rgb_logo_effect  = 0x1;      // Default: Online? to check
static uint rgb_trunk_effect = 0x1;      // Default: Online? to check
static uint rgb_left_effect  = 0x1;      // Default: 100%
static uint rgb_cntr_effect  = 0x1;      // Default: 100%
static uint rgb_right_effect = 0x1;      // Default: 100%

// Default Intensity ( << 24 )
static uint rgb_logo_level   = 0x0;       // Default: Online? to check
static uint rgb_trunk_level  = 0x1;       // Default: 50%
static uint rgb_left_level   = 0x1;       // Default: 50%
static uint rgb_cntr_level   = 0x1;       // Default: 50%
static uint rgb_right_level  = 0x1;       // Default: 50%

// endsection: Defaults


// Commit predefs
#if defined(ELUK_ENABLE_PRESETS)
static int eluk_led_wmi_colors_commit_all  (char *, const struct kernel_param *);
#endif
static int eluk_led_wmi_colors_commit_kbd  (char *, const struct kernel_param *);
static int eluk_led_wmi_colors_commit_logo (char *, const struct kernel_param *);
static int eluk_led_wmi_colors_commit_trunk(char *, const struct kernel_param *);

static int check_kbd_colors(char*);
static int check_logo_colors(char*);
static int check_trunk_colors(char*);

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

#if defined(ELUK_BUF_LOGGING)
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

void debug_print_colors(union wmi_setting* settings, int count)
{
    int counter;
    pr_info("notify:    Debugging - Printing all data\n");
    pr_info("notify:     Debugging - Printing logo   :: 0x%02X 0x%02X 0x%06X\n", rgb_logo_effect,  rgb_logo_level,  rgb_logo_color);
    pr_info("notify:     Debugging - Printing trunk  :: 0x%02X 0x%02X 0x%06X\n", rgb_trunk_effect, rgb_trunk_level, rgb_trunk_color);
    pr_info("notify:     Debugging - Printing left   :: 0x%02X 0x%02X 0x%06X\n", rgb_left_effect,  rgb_left_level,  rgb_left_color);
    pr_info("notify:     Debugging - Printing center :: 0x%02X 0x%02X 0x%06X\n", rgb_cntr_effect,  rgb_cntr_level,  rgb_cntr_color);
    pr_info("notify:     Debugging - Printing right  :: 0x%02X 0x%02X 0x%06X\n", rgb_right_effect, rgb_right_level, rgb_right_color);

    for(counter = 0; counter < count; counter++) // DON'T INFINITELOOP IN KERNEL, YOU DOPE.
    {
        eluk_led_evt_cb_buf(32*sizeof(u8), settings[counter].bytes);
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

static int eluk_led_wmi_set_value(union wmi_setting *preset, int count) {
    int it; // iterator
    bool failed = false;
    u8 size = sizeof(u8)*32; // memory size of array

    for(it = 0; ((it < count) && !failed); it++) {
        // bad, fix this
        if(eluk_shared_wmi_set_value(preset[it].bytes, size) > 0) {
            pr_info("Write failure. Please report this to the developer.\n");
            failed = true;
        }
    }
    return (failed?1:0);
}

#if defined(ELUK_ENABLE_PRESETS)

static void eluk_led_wmi_set_default_colors(void)
{
    rgb_logo_color   = 0x00FFFF;
    rgb_trunk_color  = 0x00FFFF;
    rgb_left_color   = 0xFF0000;
    rgb_cntr_color   = 0x00FF00;
    rgb_right_color  = 0x0000FF;
}

static void eluk_led_wmi_set_ambi_colors(u8 level)
{
    //rgb_logo_color   = 0x00FFFF;
    //rgb_trunk_color  = 0x00FFFF;
    rgb_left_color     = rgb_cntr_color
                       = rgb_right_color
                       = ELUK_WMI_LED_COLOR_AMBIENT;
    rgb_left_effect    = rgb_cntr_effect
                       = rgb_right_effect
                       = ELUK_WMI_LED_EFFECT_AMBIENT;
    rgb_left_level     = rgb_cntr_level
                       = rgb_right_level
                       = level;
}

static void eluk_led_wmi_set_kbd_zones_effect(u8 val)
{
    rgb_left_effect  =      val;
    rgb_cntr_effect  =      val;
    rgb_right_effect =      val;
}

static void eluk_led_wmi_set_kbd_zones_level(u8 val)
{
    rgb_left_level   =      val;
    rgb_cntr_level   =      val;
    rgb_right_level  =      val;
}

static void eluk_led_set_effect_and_level(uint effect, uint level)
{
    eluk_led_wmi_set_default_colors();
    eluk_led_wmi_set_kbd_zones_effect (effect);
    eluk_led_wmi_set_kbd_zones_level  (level);
    rgb_trunk_effect                 = effect;
    rgb_trunk_level                  = level;
    rgb_logo_effect                  = ELUK_WMI_LED_EFFECT_SOLID;
    rgb_logo_level                   = ELUK_WMI_LED_BRIGHT_NONE;
}
static int eluk_led_wmi_offline(char *buffer, const struct kernel_param *kp)
{
    // Set offline, but hard for now.
    eluk_led_set_effect_and_level(ELUK_WMI_LED_EFFECT_SOLID, ELUK_WMI_LED_BRIGHT_NONE);
    return eluk_led_wmi_colors_commit_all(NULL, NULL);
}

static int eluk_led_wmi_solid_50(char *buffer, const struct kernel_param *kp)
{
    eluk_led_set_effect_and_level(ELUK_WMI_LED_EFFECT_SOLID, ELUK_WMI_LED_BRIGHT_HALF);
    return eluk_led_wmi_colors_commit_all(NULL, NULL);
}

static int eluk_led_wmi_solid_100(char *buffer, const struct kernel_param *kp)
{
    eluk_led_set_effect_and_level(ELUK_WMI_LED_EFFECT_SOLID, ELUK_WMI_LED_BRIGHT_FULL);
    return eluk_led_wmi_colors_commit_all(NULL, NULL);
}

static int eluk_led_wmi_brth_50(char *buffer, const struct kernel_param *kp)
{
    eluk_led_set_effect_and_level(ELUK_WMI_LED_EFFECT_BREATHE, ELUK_WMI_LED_BRIGHT_HALF);
    return eluk_led_wmi_colors_commit_all(NULL, NULL);
}

static int eluk_led_wmi_brth_100(char *buffer, const struct kernel_param *kp)
{
    eluk_led_set_effect_and_level(ELUK_WMI_LED_EFFECT_BREATHE, ELUK_WMI_LED_BRIGHT_FULL);
    return eluk_led_wmi_colors_commit_all(NULL, NULL);
}

static int eluk_led_wmi_ambi_50(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_set_ambi_colors(ELUK_WMI_LED_BRIGHT_HALF);
    return eluk_led_wmi_colors_commit_kbd(NULL, NULL);
}

static int eluk_led_wmi_ambi_100(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_set_ambi_colors(ELUK_WMI_LED_BRIGHT_FULL);
    return eluk_led_wmi_colors_commit_kbd(NULL, NULL);
}
#endif

static int eluk_led_wmi_get_logo_a3(void)
{
    return BITW_A3(rgb_logo_effect, rgb_logo_level, rgb_logo_color);
}
static int eluk_led_wmi_get_trunk_a3(void)
{
    return BITW_A3(rgb_trunk_effect, rgb_trunk_level, rgb_trunk_color);
}
static int eluk_led_wmi_get_right_a3(void)
{
    return BITW_A3(rgb_right_effect, rgb_right_level, rgb_right_color);
}
static int eluk_led_wmi_get_centre_a3(void)
{
    return BITW_A3(rgb_cntr_effect, rgb_cntr_level, rgb_cntr_color);
}
static int eluk_led_wmi_get_left_a3(void)
{
    return BITW_A3(rgb_left_effect, rgb_left_level, rgb_left_color);
}

static int apply_settings(union wmi_setting *STNGS, u8 CNT, char* BUF, bool DO_COM)
{
    int status = 0;
    if(!DO_COM)
    {
#if defined(ELUK_BUF_LOGGING)
        debug_print_colors(STNGS, CNT);
#endif
        return 0;
    }
    if((status = eluk_led_wmi_set_value(STNGS, CNT)) > 0 && BUF != NULL) {
        strcpy(BUF, "failure\n");
    } else if (BUF != NULL) {
        strcpy(BUF, "success\n");
    }
    return (BUF!=NULL?strlen(BUF):status);
}

#if defined(ELUK_ENABLE_PRESETS)
static int actual_colors_commit_all(char *buffer, const struct kernel_param *kp, bool doCommit)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[5] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_LOGO,     .a3 = eluk_led_wmi_get_logo_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_TRUNK,    .a3 = eluk_led_wmi_get_trunk_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_RIGHT,    .a3 = eluk_led_wmi_get_right_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_CENTRE,   .a3 = eluk_led_wmi_get_centre_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_LEFT,     .a3 = eluk_led_wmi_get_left_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    };
    return apply_settings(settings, 5, buffer, doCommit); // returns
}
#endif

typedef enum {
    ELUK_ERR_CHECK_EFFECT,
    ELUK_ERR_CHECK_LEVEL,
    ELUK_ERR_CHECK_COLOR,
    ELUK_ERR_CHECK_OK,
} eluk_error_check;

static eluk_error_check eluk_led_wmi_verify(u8 effect, u8 level, u8 color, bool has_ambient) {
    switch (effect)
    {
        case(ELUK_WMI_LED_EFFECT_SOLID):
        case(ELUK_WMI_LED_EFFECT_BREATHE):
        case(ELUK_WMI_LED_EFFECT_RAINBOW):
        case(ELUK_WMI_LED_EFFECT_AMBIENT):
        {
            if(effect == ELUK_WMI_LED_EFFECT_AMBIENT && !has_ambient)
            {
                return ELUK_ERR_CHECK_EFFECT;
            }

            break;
        }
        
        default:
        {
            return ELUK_ERR_CHECK_EFFECT;
        }
    }
    switch (level)
    {
        case(ELUK_WMI_LED_BRIGHT_NONE):
        case(ELUK_WMI_LED_BRIGHT_HALF):
        case(ELUK_WMI_LED_BRIGHT_FULL):
        {
            break;
        }
        
        default:
        {
            return ELUK_ERR_CHECK_LEVEL;
        }
    }
    if(color > 0xFFFFFF)
    {
        return ELUK_ERR_CHECK_COLOR;
    }
    return ELUK_ERR_CHECK_OK;
}

static int actual_colors_commit_kbd(char *buffer, const struct kernel_param *kp, bool doCommit)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[3] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_RIGHT,    .a3 = eluk_led_wmi_get_right_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_CENTRE,   .a3 = eluk_led_wmi_get_centre_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_LEFT,     .a3 = eluk_led_wmi_get_left_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 },
    };
    return apply_settings(settings, 3, buffer, doCommit); // returns
}

static int actual_colors_commit_trunk(char *buffer, const struct kernel_param *kp, bool doCommit)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[1] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_TRUNK,    .a3 = eluk_led_wmi_get_trunk_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 }
    };
    return apply_settings(settings, 1, buffer, doCommit); // returns
}

static int actual_colors_commit_logo(char *buffer, const struct kernel_param *kp, bool doCommit)
{
    // If this is reached, launch commit. The input is not important.
    union wmi_setting settings[1] = {
    {.a0_op = QUANTA_WMI_MAGIC_SET_OP, .a1_tgt = QUANTA_WMI_MAGIC_SET_ARG_LED,
     .a2 = ELUK_WMI_LED_ZONE_LOGO,     .a3 = eluk_led_wmi_get_logo_a3(),
     .a4 = 0x0, .a5 = 0x0, .a6 = 0x0,  .rev0 = 0x0, .rev1 = 0x0 }
    };
    return apply_settings(settings, 1, buffer, doCommit); // returns
}

static int check_trunk_colors(char* buffer)
{
    switch(eluk_led_wmi_verify(rgb_trunk_effect, rgb_trunk_level, rgb_trunk_color, true))
    {
        case(ELUK_ERR_CHECK_EFFECT):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Left side has an unsupported effect.\n");
                return strlen(buffer);
            }
            else
            {
                return E_EFFECT;
            }
        }
        case(ELUK_ERR_CHECK_LEVEL):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Left side has an unsupported level.\n");
                return strlen(buffer);
            }
            else
            {
                return E_LEVEL;
            }
        }
        case(ELUK_ERR_CHECK_COLOR):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Left side has an unsupported color.\n");
                return strlen(buffer);
            }
            else
            {
                return E_COLOR; // create E_COLOR
            }
        }
        default: {}
    }
    return 0;
}

static int check_logo_colors(char* buffer)
{
    switch(eluk_led_wmi_verify(rgb_logo_effect, rgb_logo_level, rgb_logo_color, true))
    {
        case(ELUK_ERR_CHECK_EFFECT):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Logo has an unsupported effect.\n");
                return strlen(buffer);
            }
            else
            {
                return E_EFFECT;
            }
        }
        case(ELUK_ERR_CHECK_LEVEL):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Logo has an unsupported level.\n");
                return strlen(buffer);
            }
            else
            {
                return E_LEVEL;
            }
        }
        case(ELUK_ERR_CHECK_COLOR):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Logo has an unsupported color.\n");
                return strlen(buffer);
            }
            else
            {
                return E_COLOR; // create E_COLOR
            }
        }
        default: {}
    }
    return 0;
}

static int check_kbd_colors(char* buffer)
{
    switch(eluk_led_wmi_verify(rgb_left_effect, rgb_left_level, rgb_left_color, true))
    {
        case(ELUK_ERR_CHECK_EFFECT):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Left side has an unsupported effect.\n");
                return strlen(buffer);
            }
            else
            {
                return E_EFFECT;
            }
        }
        case(ELUK_ERR_CHECK_LEVEL):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Left side has an unsupported level.\n");
                return strlen(buffer);
            }
            else
            {
                return E_LEVEL;
            }
        }
        case(ELUK_ERR_CHECK_COLOR):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Left side has an unsupported color.\n");
                return strlen(buffer);
            }
            else
            {
                return E_COLOR; // create E_COLOR
            }
        }
        default: {}
    }
    switch(eluk_led_wmi_verify(rgb_cntr_effect, rgb_cntr_level, rgb_cntr_color, true))
    {
        case(ELUK_ERR_CHECK_EFFECT):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Centre has an unsupported effect.\n");
                return strlen(buffer);
            }
            else
            {
                return E_EFFECT;
            }
        }
        case(ELUK_ERR_CHECK_LEVEL):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Centre has an unsupported level.\n");
                return strlen(buffer);
            }
            else
            {
                return E_LEVEL;
            }
        }
        case(ELUK_ERR_CHECK_COLOR):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Centre has an unsupported color.\n");
                return strlen(buffer);
            }
            else
            {
                return E_COLOR; // create E_COLOR
            }
        }
        default: {}
    }
    switch(eluk_led_wmi_verify(rgb_right_effect, rgb_right_level, rgb_right_color, true))
    {
        case(ELUK_ERR_CHECK_EFFECT):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Right side has an unsupported effect.\n");
                return strlen(buffer);
            }
            else
            {
                return E_EFFECT;
            }
        }
        case(ELUK_ERR_CHECK_LEVEL):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Right side has an unsupported level.\n");
                return strlen(buffer);
            }
            else
            {
                return E_LEVEL;
            }
        }
        case(ELUK_ERR_CHECK_COLOR):
        {
            if(buffer != NULL) 
            {
                strcpy(buffer, "Right side has an unsupported color.\n");
                return strlen(buffer);
            }
            else
            {
                return E_COLOR; // create E_COLOR
            }
        }
        default: {}
    }
    return 0;
}

#if defined(ELUK_ENABLE_PRESETS)
static int eluk_led_wmi_colors_commit_all(char *buffer, const struct kernel_param *kp)
{
    int status;
    if(((status = check_kbd_colors(buffer)) != 0) 
        || ((status = check_logo_colors(buffer)) != 0)
        || ((status = check_trunk_colors(buffer)) != 0)
    )
    {
        return status;
    }
    return actual_colors_commit_all(buffer, kp, true);
}
/*
static int eluk_led_wmi_colors_pretend_commit_all(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_verify(rgb_left_effect, rgb_left_level, rgb_left_color, true);
    eluk_led_wmi_verify(rgb_cntr_effect, rgb_cntr_level, rgb_cntr_color, true);
    eluk_led_wmi_verify(rgb_right_effect, rgb_right_level, rgb_right_color, true);
    eluk_led_wmi_verify(rgb_logo_effect, rgb_logo_level, rgb_logo_color, false);
    eluk_led_wmi_verify(rgb_trunk_effect, rgb_trunk_level, rgb_trunk_color, false);
    return actual_colors_commit_all(buffer, kp, false);
}
*/
#endif


static int eluk_led_wmi_colors_commit_kbd(char *buffer, const struct kernel_param *kp)
{
    int status;
    if((status = check_kbd_colors(buffer)) != 0)
    {
        return status;
    }
    return actual_colors_commit_kbd(buffer, kp, true);
}

static int eluk_led_wmi_colors_commit_trunk(char *buffer, const struct kernel_param *kp)
{
    int status;
    if((status = check_trunk_colors(buffer)) != 0)
    {
        return status;
    }
    return actual_colors_commit_trunk(buffer, kp, true);
}

static int eluk_led_wmi_colors_commit_logo(char *buffer, const struct kernel_param *kp)
{
    int status;
    if((status = check_logo_colors(buffer)) != 0)
    {
        return status;
    }
    return actual_colors_commit_logo(buffer, kp, true);
}

// pretend up
#if defined(ELUK_TESTING)
static int eluk_led_wmi_colors_pretend_commit_kbd(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_verify(rgb_left_effect, rgb_left_level, rgb_left_color, true);
    eluk_led_wmi_verify(rgb_cntr_effect, rgb_cntr_level, rgb_cntr_color, true);
    eluk_led_wmi_verify(rgb_right_effect, rgb_right_level, rgb_right_color, true);
    return actual_colors_commit_kbd(buffer, kp, false);
}

static int eluk_led_wmi_colors_pretend_commit_trunk(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_verify(rgb_trunk_effect, rgb_trunk_level, rgb_trunk_color, false);
    return actual_colors_commit_trunk(buffer, kp, false);
}

static int eluk_led_wmi_colors_pretend_commit_logo(char *buffer, const struct kernel_param *kp)
{
    eluk_led_wmi_verify(rgb_logo_effect, rgb_logo_level, rgb_logo_color, false);
    return actual_colors_commit_logo(buffer, kp, false);
}
#endif

module_wmi_driver(eluk_led_wmi_driver);

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("LED functions for the Eluktronics Prometheus XVI WMI interface");
MODULE_VERSION("0.1.0");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: eluk-pxvi-shared-wmi");

// Readonly perm macros
#define PERM_W_ADMIN  (S_IWUSR | S_IWGRP)
#define PERM_RO_ALL   (S_IRUSR | S_IRGRP | S_IROTH)
#define PERM_RW_ADMIN (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP)

// section: preset ops
#if defined(ELUK_ENABLE_PRESETS)
static const struct kernel_param_ops eluk_kbd_preset_offline_ops = {
    .get    = eluk_led_wmi_offline,
    .set    = NULL,
};
module_param_cb(rgb_preset_offline, &eluk_kbd_preset_offline_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_preset_offline, "Apply 0-out RGB driver preset.");

static const struct kernel_param_ops eluk_kbd_preset_solid_50_ops = {
    .get    = eluk_led_wmi_solid_50,
    .set    = NULL,
};
module_param_cb(rgb_preset_solid_50, &eluk_kbd_preset_solid_50_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_preset_solid_50, "Apply Solid Half Brightness RGB driver preset.");
 
static const struct kernel_param_ops eluk_kbd_preset_solid_100_ops = {
    .get    = eluk_led_wmi_solid_100,
    .set    = NULL,
};
module_param_cb(rgb_preset_solid_100, &eluk_kbd_preset_solid_100_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_preset_solid_100, "Apply Solid Full Brightness RGB driver preset.");

static const struct kernel_param_ops eluk_kbd_preset_breathing_50_ops = {
    .get    = eluk_led_wmi_brth_50,
    .set    = NULL,
};
module_param_cb(rgb_preset_breathing_50, &eluk_kbd_preset_breathing_50_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_preset_breathing_50, "Apply Breathing Half Brightness RGB driver preset.");
 
static const struct kernel_param_ops eluk_kbd_preset_breathing_100_ops = {
    .get    = eluk_led_wmi_brth_100,
    .set    = NULL,
};
module_param_cb(rgb_preset_breathing_100, &eluk_kbd_preset_breathing_100_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_preset_breathing_100, "Apply Breathing Full Brightness RGB driver preset.");

static const struct kernel_param_ops eluk_kbd_preset_ambient_50_ops = {
    .get    = eluk_led_wmi_ambi_50,
    .set    = NULL,
};
module_param_cb(rgb_preset_ambient_50, &eluk_kbd_preset_ambient_50_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_preset_ambient_50, "Apply Ambilight Half Brightness RGB driver preset.");
 
static const struct kernel_param_ops eluk_kbd_preset_ambient_100_ops = {
    .get    = eluk_led_wmi_ambi_100,
    .set    = NULL,
};
module_param_cb(rgb_preset_ambient_100, &eluk_kbd_preset_ambient_100_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_preset_ambient_100, "Apply Ambilight Full Brightness RGB driver preset.");
#endif
// endsection: preset ops

// section: Zone Colors
module_param_named(rgb_set_logo_color, rgb_logo_color, uint, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_logo_color, "Color for the Logo.");

module_param_named(rgb_set_trunk_color, rgb_trunk_color, uint, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_trunk_color, "Color for the Trunk.");

module_param_named(rgb_set_left_color, rgb_left_color, uint, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_left_color, "Color for the Left.");

module_param_named(rgb_set_cntr_color, rgb_cntr_color, uint, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_cntr_color, "Color for the Center.");

module_param_named(rgb_set_right_color, rgb_right_color, uint, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_right_color, "Color for the Right.");
// endsection: Zone Colors


// section: Effect/Brightness Setting
// TODO: can these be made smaller my lord.
module_param_named(rgb_set_logo_effect, rgb_logo_effect, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_logo_effect, "Effect for the Logo.");

module_param_named(rgb_set_trunk_effect, rgb_trunk_effect, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_trunk_effect, "Effect for the Trunk.");

module_param_named(rgb_set_left_effect, rgb_left_effect, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_left_effect, "Effect for the Left.");

module_param_named(rgb_set_cntr_effect, rgb_cntr_effect, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_cntr_effect, "Effect for the Center.");

module_param_named(rgb_set_right_effect, rgb_right_effect, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_right_effect, "Effect for the Right.");

// --

module_param_named(rgb_set_logo_level, rgb_logo_level, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_logo_level, "Brightness for the Logo.");

module_param_named(rgb_set_trunk_level, rgb_trunk_level, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_trunk_level, "Brightness for the Trunk.");

module_param_named(rgb_set_left_level, rgb_left_level, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_left_level, "Brightness for the Left.");

module_param_named(rgb_set_cntr_level, rgb_cntr_level, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_cntr_level, "Brightness for the Center.");

module_param_named(rgb_set_right_level, rgb_right_level, int, PERM_RW_ADMIN);
MODULE_PARM_DESC(rgb_set_right_level, "Brightness for the Right.");
// endsection: Effect/Brightness Setting


// section: commit ops
/*
static const struct kernel_param_ops eluk_commit_all_ops = {
    .get    = eluk_led_wmi_colors_commit_all,
    .set    = NULL,
};
module_param_cb(rgb_commit_all, &eluk_commit_all_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_commit_all, "Commit all colors and mode setup to WMI.");
*/

static const struct kernel_param_ops eluk_commit_kbd_ops = {
    .get    = eluk_led_wmi_colors_commit_kbd,
    .set    = NULL,
};
module_param_cb(rgb_commit_kbd, &eluk_commit_kbd_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_commit_kbd, "Commit keyboard colors and mode setup to WMI.");

static const struct kernel_param_ops eluk_commit_trunk_ops = {
    .get    = eluk_led_wmi_colors_commit_trunk,
    .set    = NULL,
};
module_param_cb(rgb_commit_trunk, &eluk_commit_trunk_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_commit_trunk, "Commit trunk colors and mode setup to WMI.");

// Unused on Eluktronics
static const struct kernel_param_ops eluk_commit_logo_ops = {
    .get    = eluk_led_wmi_colors_commit_logo,
    .set    = NULL,
};
module_param_cb(rgb_commit_logo, &eluk_commit_logo_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_commit_logo, "Commit logo colors and mode setup to WMI.");


/*
static const struct kernel_param_ops eluk_pretend_commit_all_ops = {
    .get    = eluk_led_wmi_colors_pretend_commit_all,
    .set    = NULL,
};
module_param_cb(rgb_pretend_commit_all, &eluk_pretend_commit_all_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_pretend_commit_all, "Commit all colors and mode setup to WMI.");
*/

#if defined(ELUK_TESTING)
static const struct kernel_param_ops eluk_pretend_commit_kbd_ops = {
    .get    = eluk_led_wmi_colors_pretend_commit_kbd,
    .set    = NULL,
};
module_param_cb(rgb_pretend_commit_kbd, &eluk_pretend_commit_kbd_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_pretend_commit_kbd, "Commit keyboard colors and mode setup to WMI.");

static const struct kernel_param_ops eluk_pretend_commit_trunk_ops = {
    .get    = eluk_led_wmi_colors_pretend_commit_trunk,
    .set    = NULL,
};
module_param_cb(rgb_pretend_commit_trunk, &eluk_pretend_commit_trunk_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_pretend_commit_trunk, "Commit trunk colors and mode setup to WMI.");

// Unused on Eluktronics
static const struct kernel_param_ops eluk_pretend_commit_logo_ops = {
    .get    = eluk_led_wmi_colors_pretend_commit_logo,
    .set    = NULL,
};
module_param_cb(rgb_pretend_commit_logo, &eluk_pretend_commit_logo_ops, NULL, PERM_RO_ALL);
MODULE_PARM_DESC(rgb_pretend_commit_logo, "Commit logo colors and mode setup to WMI.");
#endif
// endsection: commit ops

MODULE_DEVICE_TABLE(wmi, eluk_led_wmi_device_ids);
MODULE_ALIAS_ELUK_LED_WMI();
