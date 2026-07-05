/*
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config-miners.h"

#include <gio/gio.h>

#include "tracker-dbus.h"

inline GBusType
tracker_ipc_bus (void)
{
	const gchar *bus = g_getenv ("TRACKER_BUS_TYPE");

	return G_UNLIKELY (g_strcmp0 (bus, "system") == 0) ? G_BUS_TYPE_SYSTEM : G_BUS_TYPE_SESSION;
}
