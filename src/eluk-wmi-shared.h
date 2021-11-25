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

#ifndef ELUK_WMI_SHARED_H
#define ELUK_WMI_SHARED_H

#include "quanta_interfaces.h"

#define ELUK_SHARED_IFACE_WMI_STRID "eluk-wmi-shared"

// This WMI ID should be the conduit to read and write to ACPI_SMI,
//  via wmi_set_block/wmi_query_block.
#define ELUK_WMI_MGMT_GUID_LED_RD_WR  "644C5791-B7B0-4123-A90B-E93876E0DAAD"

// This WMI ID is an event conduit from which the system notifies the driver.
#define ELUK_WMI_EVNT_GUID_MESG_MNTR  "74286D6E-429C-427A-B34B-B5D15D032B05"

#define MODULE_ALIAS_ELUK_SHARED_WMI() \
    MODULE_ALIAS("wmi:" ELUK_WMI_MGMT_GUID_LED_RD_WR); \
    MODULE_ALIAS("wmi:" ELUK_WMI_EVNT_GUID_MESG_MNTR);

#endif