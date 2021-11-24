/*!
 * Copyright (c) 2021 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 *
 * This file is part of tuxedo-keyboard.
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


// start: unused?
#define   QUANTA_EC_REG_LDAT	0x8a
#define   QUANTA_EC_REG_HDAT	0x8b
#define   QUANTA_EC_REG_FLAGS	0x8c
#define   QUANTA_EC_REG_CMDL	0x8d
#define   QUANTA_EC_REG_CMDH	0x8e

#define   QUANTA_EC_BIT_RFLG	0
#define   QUANTA_EC_BIT_WFLG	1
#define   QUANTA_EC_BIT_BFLG	2
#define   QUANTA_EC_BIT_CFLG	3
#define   QUANTA_EC_BIT_DRDY	7

#define   QNT_EC_WAIT_CYCLES	0x50

//  stop: unused?

static u8 baseline_solid[5][32] = {
	//  0     1     2     3     4     5     6     7     8     9    10    11
	{0x00, 0xFB, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // trunk/logo?
	{0x00, 0xFB, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // logo/trunk?
	//{0x00, 0xFB, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x15, 0xFF, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led3 // red to right
	{0x00, 0xFB, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0xFF, 0x15, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led3 // blue to right
	{0x00, 0xFB, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led2
	{0x00, 0xFB, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0xFF, 0x06, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led1
};

static u8 baseline_breathing_50[5][32] = {
	//  0     1     2     3     4     5     6     7     8     9    10    11
	{0x00, 0xFB, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // trunk/logo?
	{0x00, 0xFB, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // logo/trunk?
	{0x00, 0xFB, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x15, 0xFF, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led3 (right)
	{0x00, 0xFB, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led2 (centre)
	{0x00, 0xFB, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0xFF, 0x06, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led1 (left)
};
static u8 baseline_breathing_100[5][32] = {
	//  0     1     2     3     4     5     6     7     8     9    10    11
	{0x00, 0xFB, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // trunk/logo?
	{0x00, 0xFB, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // logo/trunk?
	{0x00, 0xFB, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x15, 0xFF, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led3 (right)
	{0x00, 0xFB, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led2 (centre)
	{0x00, 0xFB, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0xFF, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // led1 (left)
};


DEFINE_MUTEX(eluk_wmi_lock); // unused?

struct quanta_interface_t eluk_led_wmi_interface = {
	.string_id = ELUK_LED_INTERFACE_WMI_STRID,
};

static void eluk_led_wmi_run_query(void)
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
			quanta_event_callb_buf(out_acpi->buffer.length, out_acpi->buffer.pointer);
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

	quanta_add_interface(ELUK_LED_INTERFACE_WMI_STRID, &eluk_led_wmi_interface);

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
	quanta_remove_interface(ELUK_LED_INTERFACE_WMI_STRID, &eluk_led_wmi_interface);
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
		if (!IS_ERR_OR_NULL(eluk_led_wmi_interface.event_callb_int)) {
			u32 code;
			code = obj->integer.value;
			// Execute registered callback
			eluk_led_wmi_interface.event_callb_int(code);
		} else {
			pr_debug("no registered callback\n");
		}
	} else if (obj->type == ACPI_TYPE_BUFFER) {
		if (!IS_ERR_OR_NULL(eluk_led_wmi_interface.event_callb_buf)) {
			// Execute registered callback
			eluk_led_wmi_interface.event_callb_buf(obj->buffer.length, obj->buffer.pointer);
		} else {
			pr_debug("no registered callback\n");
		}
	} else {
		pr_debug("unknown event type - %d (%0#6x)\n", obj->type, obj->type);
	}
}

void quanta_event_callb_buf(u8 b_l, u8* b_ptr)
{
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
		.name	= ELUK_LED_INTERFACE_WMI_STRID,
		.owner	= THIS_MODULE
	},
	.id_table	= eluk_led_wmi_device_ids,
	.probe		= eluk_led_wmi_probe,
	.remove		= eluk_led_wmi_remove,
	.notify		= eluk_led_wmi_notify,
};

#if 1
#define RUN_THE_TEST
#endif

static int eluk_led_wmi_set_value(const char *val, const struct kernel_param *kp)
{
#if defined(RUN_THE_TEST)
	struct acpi_buffer input;
#endif
	acpi_status status;
	int iter = 0;
	bool failed = false;
	/*
	char valcp[16];
	char *s;

	strncpy(valcp, val, 16);
	valcp[15] = '\0';

	s = strstrip(valcp);
	*/

	// don't care about it actually for now
	pr_info("Hi userspace how are you.\n");

	for(iter; ((iter < 5) && !failed); iter++) {
		pr_info("Will write config to the wmi, maybe.\n");
#if defined(RUN_THE_TEST)
		input.length = (sizeof(u8)*32); // u8 array
		input.pointer = baseline_solid[iter];
		status = wmi_set_block(ELUK_WMI_MGMT_GUID_LED_RD_WR, 0, &input);
		if (ACPI_FAILURE(status)) {
			pr_info("Write fail, meh. We're debugging at this point, reboot to windows to fix the state?\n");
			failed = true;
		} else {
			// immediate read?
			eluk_led_wmi_run_query();
		}
#else
		pr_info("Would have written. Still need to be sure I won't break things. Would have written this:\n");
		quanta_event_callb_buf(32, baseline_solid[iter]);
#endif
		/*
		
		*/
	}

	return 0;
}

static int eluk_led_wmi_get_value(char *buffer, const struct kernel_param *kp) {
	strcpy(buffer, "fake get\n");
	return strlen(buffer);
}

static int eluk_wmi_read_value(char *buffer, const struct kernel_param *kp) {
	strcpy(buffer, "fake read\n");
	return strlen(buffer);
}

module_wmi_driver(eluk_led_wmi_driver);

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("Driver for Quanta-Based Eluktronics WMI interface, based on TUXEDO code");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");

/**
 * @brief Kernel ops interaction test
 */
static const struct kernel_param_ops eluk_read_op = {
	.set	= NULL,
	.get	= eluk_wmi_read_value,
};
module_param_cb(eluk_read, &eluk_read_op, NULL, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(eluk_read, "Read Only.");

static const struct kernel_param_ops eluk_trigger_ops = {
	.set	= eluk_led_wmi_set_value,
	.get	= eluk_led_wmi_get_value,
};
module_param_cb(eluk_trigger, &eluk_trigger_ops, NULL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(eluk_trigger, "Trigger testing. Read and Write.");

MODULE_DEVICE_TABLE(wmi, eluk_led_wmi_device_ids);
MODULE_ALIAS_ELUK_LED_WMI();
