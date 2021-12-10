/*!
 * Copyright (c) 2021 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 * Parts of this file Copyright (c) 2021 Renaud Lepage <root@cybikbase.com>
 *
 * This file is part of eluk-pxvi-wmi, an offshoot of tuxedo-keyboard, and heavily
 * modified to be nigh unrecognizable from the original. Notices will be
 * preserved per the spirit of Open Source development.
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
#ifndef ELUK_SHARED_DEFS_INTERFACES_H
#define ELUK_SHARED_DEFS_INTERFACES_H

#include "eluk-pxvi-shared-wmi.h"

//#define ELUK_DEBUGGING
//#define ELUK_BUF_LOGGING
//#define ELUK_ENABLE_PRESETS

static struct eluk_shared_interfaces_t {
    struct eluk_shared_interface_t *wmi;
} eluk_shared_interfaces = { .wmi = NULL };

static DEFINE_MUTEX(eluk_shared_interface_modification_lock);

u32 eluk_shared_add_interface(const char* interface_name,
                         struct eluk_shared_interface_t *interface)
{
    mutex_lock(&eluk_shared_interface_modification_lock);

    if (strcmp(interface->string_id, interface_name) == 0) {
        eluk_shared_interfaces.wmi = interface;
    } else {
        TUXEDO_DEBUG("trying to add unknown interface\n");
        mutex_unlock(&eluk_shared_interface_modification_lock);
        return -EINVAL;
    }
    interface->evt_cb_int = eluk_shared_evt_cb_int;
    interface->evt_cb_buf = eluk_shared_evt_cb_buf;

    mutex_unlock(&eluk_shared_interface_modification_lock);

    return 0;
}
EXPORT_SYMBOL(eluk_shared_add_interface);

u32 eluk_shared_remove_interface(const char* interface_name, 
                            struct eluk_shared_interface_t *interface)
{
    mutex_lock(&eluk_shared_interface_modification_lock);

    if (strcmp(interface->string_id, interface_name) == 0) {
        eluk_shared_interfaces.wmi = NULL;
    } else {
        mutex_unlock(&eluk_shared_interface_modification_lock);
        return -EINVAL;
    }

    mutex_unlock(&eluk_shared_interface_modification_lock);

    return 0;
}
EXPORT_SYMBOL(eluk_shared_remove_interface);

u32 eluk_shared_get_active_interface_id(char **id_str)
{
    if (IS_ERR_OR_NULL(eluk_shared_interfaces.wmi))
        return -ENODEV;

    if (!IS_ERR_OR_NULL(id_str))
        *id_str = eluk_shared_interfaces.wmi->string_id;

    return 0;
}
EXPORT_SYMBOL(eluk_shared_get_active_interface_id);

void eluk_shared_evt_cb_int(u32 code)
{
    // NOOP
}

#if !defined(ELUK_DEBUGGING)
#endif

#endif