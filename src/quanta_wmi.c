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
#include "quanta_interfaces.h"

#define QUANTA_EC_REG_LDAT	0x8a
#define QUANTA_EC_REG_HDAT	0x8b
#define QUANTA_EC_REG_FLAGS	0x8c
#define QUANTA_EC_REG_CMDL	0x8d
#define QUANTA_EC_REG_CMDH	0x8e

#define QUANTA_EC_BIT_RFLG	0
#define QUANTA_EC_BIT_WFLG	1
#define QUANTA_EC_BIT_BFLG	2
#define QUANTA_EC_BIT_CFLG	3
#define QUANTA_EC_BIT_DRDY	7

#define QNT_EC_WAIT_CYCLES	0x50

//static bool quanta_ec_direct = true;
static bool quanta_ec_direct = false;

DEFINE_MUTEX(quanta_ec_lock);

static u32 qnt_wmi_ec_evaluate(u8 addr_low, u8 addr_high, u8 data_low, u8 data_high, u8 read_flag, u32 *return_buffer)
{
	acpi_status status;
	union acpi_object *out_acpi;
	u32 e_result = 0;

	// Kernel buffer for input argument
	u32 *wmi_arg = (u32 *) kmalloc(sizeof(u32)*10, GFP_KERNEL);
	// Byte reference to the input buffer
	u8 *wmi_arg_bytes = (u8 *) wmi_arg;

	u8 wmi_instance = 0x00;
	u32 wmi_method_id = 0x04;
	struct acpi_buffer wmi_in = { (acpi_size) sizeof(wmi_arg), wmi_arg};
	struct acpi_buffer wmi_out = { ACPI_ALLOCATE_BUFFER, NULL };

	mutex_lock(&quanta_ec_lock);

	// Zero input buffer
	memset(wmi_arg, 0x00, 10 * sizeof(u32));

	// Configure the input buffer
	wmi_arg_bytes[0] = addr_low;
	wmi_arg_bytes[1] = addr_high;
	wmi_arg_bytes[2] = data_low;
	wmi_arg_bytes[3] = data_high;

	if (read_flag != 0) {
		wmi_arg_bytes[5] = 0x01;
	}
	
	status = wmi_evaluate_method(QUANTA_WMI_MGMT_GUID_BA, wmi_instance, wmi_method_id, &wmi_in, &wmi_out);
	out_acpi = (union acpi_object *) wmi_out.pointer;

	if (out_acpi && out_acpi->type == ACPI_TYPE_BUFFER) {
		memcpy(return_buffer, out_acpi->buffer.pointer, out_acpi->buffer.length);
	} /* else if (out_acpi && out_acpi->type == ACPI_TYPE_INTEGER) {
		e_result = (u32) out_acpi->integer.value;
	}*/
	if (ACPI_FAILURE(status)) {
		pr_err("quanta_wmi.c: Error evaluating method\n");
		e_result = -EIO;
	}

	kfree(out_acpi);
	kfree(wmi_arg);

	mutex_unlock(&quanta_ec_lock);

	return e_result;
}

/**
 * EC address read through WMI
 */
static u32 qnt_ec_read_addr_wmi(u8 addr_low, u8 addr_high, union qnt_ec_read_return *output)
{
	u32 qnt_data[10];
	u32 ret = qnt_wmi_ec_evaluate(addr_low, addr_high, 0x00, 0x00, 1, qnt_data);
	output->dword = qnt_data[0];
	// pr_debug("addr: 0x%02x%02x value: %0#4x (high: %0#4x) result: %d\n", addr_high, addr_low, output->bytes.data_low, output->bytes.data_high, ret);
	return ret;
}

/**
 * EC address write through WMI
 */
static u32 qnt_ec_write_addr_wmi(u8 addr_low, u8 addr_high, u8 data_low, u8 data_high, union qnt_ec_write_return *output)
{
	u32 qnt_data[10];
	u32 ret = qnt_wmi_ec_evaluate(addr_low, addr_high, data_low, data_high, 0, qnt_data);
	output->dword = qnt_data[0];
	return ret;
}

/**
 * Direct EC address read
 */
static u32 qnt_ec_read_addr_direct(u8 addr_low, u8 addr_high, union qnt_ec_read_return *output)
{
	u32 result;
	u8 tmp, count, flags;

	mutex_lock(&quanta_ec_lock);

	ec_write(QUANTA_EC_REG_LDAT, addr_low);
	ec_write(QUANTA_EC_REG_HDAT, addr_high);

	flags = (0 << QUANTA_EC_BIT_DRDY) | (1 << QUANTA_EC_BIT_RFLG);
	ec_write(QUANTA_EC_REG_FLAGS, flags);

	// Wait for ready flag
	count = QNT_EC_WAIT_CYCLES;
	ec_read(QUANTA_EC_REG_FLAGS, &tmp);
	while (((tmp & (1 << QUANTA_EC_BIT_DRDY)) == 0) && count != 0) {
		msleep(1);
		ec_read(QUANTA_EC_REG_FLAGS, &tmp);
		count -= 1;
	}

	if (count != 0) {
		output->dword = 0;
		ec_read(QUANTA_EC_REG_CMDL, &tmp);
		output->bytes.data_low = tmp;
		ec_read(QUANTA_EC_REG_CMDH, &tmp);
		output->bytes.data_high = tmp;
		result = 0;
	} else {
		output->dword = 0xfefefefe;
		result = -EIO;
	}

	ec_write(QUANTA_EC_REG_FLAGS, 0x00);

	mutex_unlock(&quanta_ec_lock);

	// pr_debug("addr: 0x%02x%02x value: %0#4x result: %d\n", addr_high, addr_low, output->bytes.data_low, result);

	return result;
}

static u32 qnt_ec_write_addr_direct(u8 addr_low, u8 addr_high, u8 data_low, u8 data_high, union qnt_ec_write_return *output)
{
	u32 result = 0;
	u8 tmp, count, flags;

	mutex_lock(&quanta_ec_lock);

	ec_write(QUANTA_EC_REG_LDAT, addr_low);
	ec_write(QUANTA_EC_REG_HDAT, addr_high);
	ec_write(QUANTA_EC_REG_CMDL, data_low);
	ec_write(QUANTA_EC_REG_CMDH, data_high);

	flags = (0 << QUANTA_EC_BIT_DRDY) | (1 << QUANTA_EC_BIT_WFLG);
	ec_write(QUANTA_EC_REG_FLAGS, flags);

	// Wait for ready flag
	count = QNT_EC_WAIT_CYCLES;
	ec_read(QUANTA_EC_REG_FLAGS, &tmp);
	while (((tmp & (1 << QUANTA_EC_BIT_DRDY)) == 0) && count != 0) {
		msleep(1);
		ec_read(QUANTA_EC_REG_FLAGS, &tmp);
		count -= 1;
	}

	// Replicate wmi output depending on success
	if (count != 0) {
		output->bytes.addr_low = addr_low;
		output->bytes.addr_high = addr_high;
		output->bytes.data_low = data_low;
		output->bytes.data_high = data_high;
		result = 0;
	} else {
		output->dword = 0xfefefefe;
		result = -EIO;
	}

	ec_write(QUANTA_EC_REG_FLAGS, 0x00);

	mutex_unlock(&quanta_ec_lock);

	return result;
}

u32 qnt_wmi_read_ec_ram(u16 addr, u8 *data)
{
	u32 result;
	u8 addr_low, addr_high;
	union qnt_ec_read_return output;

	if (IS_ERR_OR_NULL(data))
		return -EINVAL;

	addr_low = addr & 0xff;
	addr_high = (addr >> 8) & 0xff;

	if (quanta_ec_direct) {
		result = qnt_ec_read_addr_direct(addr_low, addr_high, &output);
	} else {
		result = qnt_ec_read_addr_wmi(addr_low, addr_high, &output);
	}

	*data = output.bytes.data_low;
	return result;
}

u32 qnt_wmi_write_ec_ram(u16 addr, u8 data)
{
	u32 result;
	u8 addr_low, addr_high, data_low, data_high;
	union qnt_ec_write_return output;

	addr_low = addr & 0xff;
	addr_high = (addr >> 8) & 0xff;
	data_low = data;
	data_high = 0x00;

	if (quanta_ec_direct)
		result = qnt_ec_write_addr_direct(addr_low, addr_high, data_low, data_high, &output);
	else
		result = qnt_ec_write_addr_wmi(addr_low, addr_high, data_low, data_high, &output);

	return result;
}

struct quanta_interface_t quanta_wmi_interface = {
	.string_id = QUANTA_INTERFACE_WMI_STRID,
	.read_ec_ram = qnt_wmi_read_ec_ram,
	.write_ec_ram = qnt_wmi_write_ec_ram
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
static int quanta_wmi_probe(struct wmi_device *wdev)
#else
static int quanta_wmi_probe(struct wmi_device *wdev, const void *dummy_context)
#endif
{
	int status;

	// Look for for GUIDs used on Quanta-based devices
	status =
		wmi_has_guid(QUANTA_WMI_EVENT_GUID_0) &&
		wmi_has_guid(QUANTA_WMI_MGMT_GUID_BA);
	
	if (!status) {
		pr_debug("probe: At least one Quanta GUID missing\n"); // more than one?
		return -ENODEV;
	}

	quanta_add_interface(&quanta_wmi_interface);

	pr_info("interface initialized\n");

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static int  quanta_wmi_remove(struct wmi_device *wdev)
#else
static void quanta_wmi_remove(struct wmi_device *wdev)
#endif
{
	pr_debug("quanta_wmi driver remove\n");
	quanta_remove_interface(&quanta_wmi_interface);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	return 0;
#endif
}

static void quanta_wmi_notify(struct wmi_device *wdev, union acpi_object *obj)
{
	u32 code;

	if (!IS_ERR_OR_NULL(quanta_wmi_interface.event_callb)) {
		if (obj) {
			if (obj->type == ACPI_TYPE_INTEGER) {
				code = obj->integer.value;
				// Execute registered callback
				quanta_wmi_interface.event_callb(code);
			} else {
				pr_debug("unknown event type - %d (%0#6x)\n", obj->type, obj->type);
			}
		} else {
			pr_debug("expected ACPI object doesn't exist\n");
		}
	} else {
		pr_debug("no registered callback\n");
	}
}

static const struct wmi_device_id quanta_wmi_device_ids[] = {
	// Listing one should be enough, for a driver that "takes care of all anyways"
	// also prevents probe (and handling) per "device"
	{ .guid_string = QUANTA_WMI_EVENT_GUID_0 },
	{ }
};

static struct wmi_driver quanta_wmi_driver = {
	.driver = {
		.name	= QUANTA_INTERFACE_WMI_STRID,
		.owner	= THIS_MODULE
	},
	.id_table	= quanta_wmi_device_ids,
	.probe		= quanta_wmi_probe,
	.remove		= quanta_wmi_remove,
	.notify		= quanta_wmi_notify,
};

module_wmi_driver(quanta_wmi_driver);

MODULE_AUTHOR("Renaud Lepage <root@cybikbase.com>");
MODULE_DESCRIPTION("Driver for Quanta-Based WMI interface, based on TUXEDO code");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");

/*
 * If set to true, the module will use the replicated WMI functions
 * (direct ec_read/ec_write) to read and write to the EC RAM instead
 * of the original. Since the original functions, in all observed cases,
 * use excessive delays, they are not preferred.
 */
module_param_cb(ec_direct_io, &param_ops_bool, &quanta_ec_direct, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(ec_direct_io, "Do not use WMI methods to read/write EC RAM (default: true).");

MODULE_DEVICE_TABLE(wmi, quanta_wmi_device_ids);
MODULE_ALIAS_QUANTA_WMI();
