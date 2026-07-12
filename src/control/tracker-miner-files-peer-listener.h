/*
 * Copyright (C) 2015, Carlos Garnacho
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#ifndef __TRACKER_MINER_FILES_PEER_LISTENER_H__
#define __TRACKER_MINER_FILES_PEER_LISTENER_H__

#include <glib-object.h>
#include <gio/gio.h>
#include <tracker-common.h>

#define TRACKER_TYPE_MINER_FILES_PEER_LISTENER (tracker_miner_files_peer_listener_get_type ())
G_DECLARE_FINAL_TYPE (TrackerMinerFilesPeerListener,
                      tracker_miner_files_peer_listener,
                      TRACKER, MINER_FILES_PEER_LISTENER,
                      GObject)

TrackerMinerFilesPeerListener *
         tracker_miner_files_peer_listener_new          (GDBusConnection               *connection);

void     tracker_miner_files_peer_listener_add_watch    (TrackerMinerFilesPeerListener *listener,
                                                         const gchar                   *dbus_name,
                                                         GFile                         *file,
                                                         const gchar * const           *graphs,
                                                         TrackerIndexLocationFlags      flags);

void     tracker_miner_files_peer_listener_remove_dbus_name (TrackerMinerFilesPeerListener *listener,
                                                             const gchar                   *dbus_name);
gboolean tracker_miner_files_peer_listener_is_file_watched  (TrackerMinerFilesPeerListener *listener,
                                                             GFile                         *file);

#endif /* __TRACKER_MINER_FILES_PEER_LISTENER_H__ */
