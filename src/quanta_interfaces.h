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
#ifndef QUANTA_INTERFACES_H
#define QUANTA_INTERFACES_H

#include <linux/types.h>


/* ::::  Module specific Constants and simple Macros   :::: */
#define __TUXEDO_PR(lvl, fmt, ...) do { pr_##lvl(fmt, ##__VA_ARGS__); } while (0)
#define TUXEDO_INFO(fmt, ...) __TUXEDO_PR(info, fmt, ##__VA_ARGS__)
#define TUXEDO_ERROR(fmt, ...) __TUXEDO_PR(err, fmt, ##__VA_ARGS__)
#define TUXEDO_DEBUG(fmt, ...) __TUXEDO_PR(debug, "[%s:%u] " fmt, __func__, __LINE__, ##__VA_ARGS__)


// doc to understand wtf
//  https://github.com/microsoft/Windows-driver-samples/blob/master/wmi/wmiacpi/device.asl#L48
/**
 * Originals

#define QUANTA_WMI_MGMT_GUID_BA	"ABBC0F6D-8EA1-11D1-00A0-C90629100000"
#define QUANTA_WMI_MGMT_GUID_BB	"ABBC0F6E-8EA1-11D1-00A0-C90629100000"
#define QUANTA_WMI_MGMT_GUID_BC	"ABBC0F6F-8EA1-11D1-00A0-C90629100000"

#define QUANTA_WMI_EVENT_GUID_0	"ABBC0F70-8EA1-11D1-00A0-C90629100000"
#define QUANTA_WMI_EVENT_GUID_1	"ABBC0F71-8EA1-11D1-00A0-C90629100000"
#define QUANTA_WMI_EVENT_GUID_2	"	"
 */
// based on Quanta decompile, this WMI ID is the conduit to write to ACPI_SMI
#define QUANTA_WMI_MGMT_GUID_LED_RD_WR  "644C5791-B7B0-4123-A90B-E93876E0DAAD" // AA ObjectID. Said to be a method.

// based on Quanta decompile, this WMI ID is the communication *TO* the OS
//  reference: MonitorWMIACPIEvent()
#define QUANTA_WMI_EVNT_GUID_MESG_MNTR  "74286D6E-429C-427A-B34B-B5D15D032B05"

#define MODULE_ALIAS_QUANTA_WMI() \
	MODULE_ALIAS("wmi:" QUANTA_WMI_MGMT_GUID_LED_RD_WR);
	MODULE_ALIAS("wmi:" QUANTA_WMI_EVNT_GUID_MESG_MNTR);

#define QUANTA_INTERFACE_WMI_STRID "eluk-led-wmi"

typedef u32 (quanta_read_ec_ram_t)(u16, u8*);
typedef u32 (quanta_write_ec_ram_t)(u16, u8);
typedef void (quanta_event_callb_int_t)(u32);
typedef void (quanta_event_callb_buf_t)(u8, u8*);

struct quanta_interface_t {
	char *string_id;
	quanta_event_callb_int_t *event_callb_int;
	quanta_event_callb_buf_t *event_callb_buf;
	quanta_read_ec_ram_t *read_ec_ram;
	quanta_write_ec_ram_t *write_ec_ram;
};

u32 quanta_add_interface(struct quanta_interface_t *new_interface);
u32 quanta_remove_interface(struct quanta_interface_t *interface);
quanta_read_ec_ram_t quanta_read_ec_ram;
quanta_write_ec_ram_t quanta_write_ec_ram;
u32 quanta_get_active_interface_id(char **id_str);

union qnt_ec_read_return {
	u32 dword;
	struct {
		u8 data_low;
		u8 data_high;
	} bytes;
};

union qnt_ec_write_return {
	u32 dword;
	struct {
		u8 addr_low;
		u8 addr_high;
		u8 data_low;
		u8 data_high;
	} bytes;
};

static struct quanta_interfaces_t {
	struct quanta_interface_t *wmi;
} quanta_interfaces = { .wmi = NULL };

quanta_event_callb_int_t quanta_event_callb_int;
quanta_event_callb_buf_t quanta_event_callb_buf;


u32 quanta_read_ec_ram(u16 address, u8 *data)
{
	u32 status;

	if (!IS_ERR_OR_NULL(quanta_interfaces.wmi)) {
		pr_info("quanta: reading\n");
		status = quanta_interfaces.wmi->read_ec_ram(address, data);
	} else {
		pr_err("no active interface while read addr 0x%04x\n", address);
		status = -EIO;
	}

	return status;
}
EXPORT_SYMBOL(quanta_read_ec_ram);

u32 quanta_write_ec_ram(u16 address, u8 data)
{
	u32 status;

	if (!IS_ERR_OR_NULL(quanta_interfaces.wmi)) {
		pr_info("quanta: writing\n");
		status = quanta_interfaces.wmi->write_ec_ram(address, data);
	} else {
		pr_err("no active interface while write addr 0x%04x data 0x%02x\n", address, data);
		status = -EIO;
	}

	return status;
}
EXPORT_SYMBOL(quanta_write_ec_ram);


static DEFINE_MUTEX(quanta_interface_modification_lock);

u32 quanta_add_interface(struct quanta_interface_t *interface)
{
	mutex_lock(&quanta_interface_modification_lock);

	if (strcmp(interface->string_id, QUANTA_INTERFACE_WMI_STRID) == 0) {
		quanta_interfaces.wmi = interface;
	} else {
		TUXEDO_DEBUG("trying to add unknown interface\n");
		mutex_unlock(&quanta_interface_modification_lock);
		return -EINVAL;
	}
	interface->event_callb_int = quanta_event_callb_int;
	interface->event_callb_buf = quanta_event_callb_buf;

	mutex_unlock(&quanta_interface_modification_lock);

	// Initialize driver if not already present
	//tuxedo_keyboard_init_driver(&quanta_keyboard_driver);

	return 0;
}
EXPORT_SYMBOL(quanta_add_interface);

u32 quanta_remove_interface(struct quanta_interface_t *interface)
{
	mutex_lock(&quanta_interface_modification_lock);

	if (strcmp(interface->string_id, QUANTA_INTERFACE_WMI_STRID) == 0) {
		// Remove driver if last interface is removed
		//tuxedo_keyboard_remove_driver(&quanta_keyboard_driver);

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

void quanta_event_callb_int(u32 code)
{
	// NOOP
}
/*
void quanta_event_callb(u32 code)
{
	if (quanta_keyboard_driver.input_device != NULL)
		if (!sparse_keymap_report_known_event(quanta_keyboard_driver.input_device, code, 1, true)) {
			TUXEDO_DEBUG("Unknown code - %d (%0#6x)\n", code, code);
		}

	// Special key combination when mode change key is pressed
	if (code == 0xb0) {
		input_report_key(quanta_keyboard_driver.input_device, KEY_LEFTMETA, 1);
		input_report_key(quanta_keyboard_driver.input_device, KEY_LEFTALT, 1);
		input_report_key(quanta_keyboard_driver.input_device, KEY_F6, 1);
		input_sync(quanta_keyboard_driver.input_device);
		input_report_key(quanta_keyboard_driver.input_device, KEY_F6, 0);
		input_report_key(quanta_keyboard_driver.input_device, KEY_LEFTALT, 0);
		input_report_key(quanta_keyboard_driver.input_device, KEY_LEFTMETA, 0);
		input_sync(quanta_keyboard_driver.input_device);
	}

	// Keyboard backlight brightness toggle
	if (quanta_kbd_bl_type_rgb_single_color) {
		switch (code) {
		case UNIWILL_OSD_KB_LED_LEVEL0:
			kbd_led_state_uw.brightness = 0x00;
			quanta_write_kbd_bl_state();
			break;
		case UNIWILL_OSD_KB_LED_LEVEL1:
			kbd_led_state_uw.brightness = 0x20;
			quanta_write_kbd_bl_state();
			break;
		case UNIWILL_OSD_KB_LED_LEVEL2:
			kbd_led_state_uw.brightness = 0x50;
			quanta_write_kbd_bl_state();
			break;
		case UNIWILL_OSD_KB_LED_LEVEL3:
			kbd_led_state_uw.brightness = 0x80;
			quanta_write_kbd_bl_state();
			break;
		case UNIWILL_OSD_KB_LED_LEVEL4:
			kbd_led_state_uw.brightness = 0xc8;
			quanta_write_kbd_bl_state();
			break;
		// Also refresh keyboard state on cable switch event
		case UNIWILL_OSD_DC_ADAPTER_CHANGE:
			quanta_write_kbd_bl_state();
			break;
		}
	}
}
*/

#endif
