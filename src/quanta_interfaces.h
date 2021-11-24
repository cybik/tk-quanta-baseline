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



// The following GUIDs were discovered on the basis of analysis of Quanta's tool and WMI sleuthing,
//  However, these were encountered in the wild only on an Eluktronics device as of this writing.
//
// Validating this WMI with other Quantas, such as the Casper Excalibur G911, require investment.
//  Thus, let "downstream" declare the WMI UDIDs for now.

// This WMI ID should be the conduit to write to ACPI_SMI, and read from it as well, 
//  via set_block/query_block.
//#define QUANTA_WMI_MGMT_GUID_LED_RD_WR  "644C5791-B7B0-4123-A90B-E93876E0DAAD"

// This WMI ID is the communication *TO* the OS
//#define QUANTA_WMI_EVNT_GUID_MESG_MNTR  "74286D6E-429C-427A-B34B-B5D15D032B05"

// Here to base newer modules off of.
/*
#define MODULE_ALIAS_QUANTA_WMI() \
	MODULE_ALIAS("wmi:" QUANTA_WMI_MGMT_GUID_LED_RD_WR); \
	MODULE_ALIAS("wmi:" QUANTA_WMI_EVNT_GUID_MESG_MNTR);
*/

// Please refer to the QUANTA_INTERFACES.md document for information on the magicks.

#define QUANTA_WMI_MAGIC_NUMBER_GET_BLOCK		0xFA00 // 64000
#define QUANTA_WMI_MAGIC_NUMBER_GET_LEDS		0x0100 //   256
#define QUANTA_WMI_MAGIC_NUMBER_GET_HW_INFO		0x0200 //   512
#define QUANTA_WMI_MAGIC_NUMBER_GET_BIOS_VER	0x0201 //   513

#define QUANTA_WMI_MAGIC_NUMBER_SET_BLOCK		0xFB00 // 64256
#define QUANTA_WMI_MAGIC_NUMBER_SET_PRETTY		0x0100 //   256
#define QUANTA_WMI_MAGIC_NUMBER_SET_WIN_KEY		0x0200 //   512
#define QUANTA_WMI_MAGIC_NUMBER_SET_PWR_MODE	0x0300 //   768

struct wmi_setting_struct {
    u16 wmi_setting_a1_operation;	// a1, enum?
    u16 wmi_setting_a2_op_target;	// a2, enum?
	u32 wmi_setting_arg3;			// a3
	u32 wmi_setting_arg4;			// a4
	u32 wmi_setting_arg5;			// a5
	u32 wmi_setting_arg6;			// a6
	u32 wmi_setting_rev0;			// unused
	u32 wmi_setting_rev1;			// unused
} __attribute__((packed));

typedef void (quanta_event_callb_int_t)(u32);
typedef void (quanta_event_callb_buf_t)(u8, u8*);

struct quanta_interface_t {
	char *string_id;
	quanta_event_callb_int_t *event_callb_int;
	quanta_event_callb_buf_t *event_callb_buf;
};

u32 quanta_add_interface(const char* name, struct quanta_interface_t *new_interface);
u32 quanta_remove_interface(const char* name, struct quanta_interface_t *interface);
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
	interface->event_callb_int = quanta_event_callb_int;
	interface->event_callb_buf = quanta_event_callb_buf;

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

void quanta_event_callb_int(u32 code)
{
	// NOOP
}

#endif
