/*
 * Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
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
 */

#include "config-miners.h"

#include <tracker-common.h>

#include "tracker-dbus-files-index.h"
#include "tracker-dbus-files-proxy.h"
#include "tracker-miner-files-index.h"
#include "tracker-miner-files-peer-listener.h"

struct _TrackerMinerFilesIndex {
	GObject parent_instance;

	TrackerMinerFilesPeerListener *peer_listener;
	TrackerDBusMinerFilesIndex *skeleton;
	TrackerDBusMinerFilesProxy *proxy_skeleton;
	GDBusConnection *d_connection;
	GArray *indexed_files;
	GStrv graphs;
	gchar *full_path;
};

enum {
	CLOSE,
	N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

static void tracker_miner_files_index_initable_iface_init (GInitableIface *iface);

static void     index_finalize            (GObject              *object);

G_DEFINE_TYPE_WITH_CODE (TrackerMinerFilesIndex, tracker_miner_files_index, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                tracker_miner_files_index_initable_iface_init))

static void
tracker_miner_files_index_class_init (TrackerMinerFilesIndexClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = index_finalize;

	signals[CLOSE] =
		g_signal_new ("close",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST, 0,
		              NULL, NULL, NULL,
		              G_TYPE_NONE, 0);
}

static void
index_finalize (GObject *object)
{
	TrackerMinerFilesIndex *files_index =
		TRACKER_MINER_FILES_INDEX (object);

	g_object_unref (files_index->skeleton);
	g_object_unref (files_index->proxy_skeleton);

	g_clear_object (&files_index->d_connection);
	g_clear_object (&files_index->peer_listener);
	g_array_unref (files_index->indexed_files);
	g_free (files_index->full_path);
	g_strfreev (files_index->graphs);

	G_OBJECT_CLASS (tracker_miner_files_index_parent_class)->finalize (object);
}

static TrackerIndexLocationFlags
parse_index_location_flags (const gchar * const *flags_strv)
{
	TrackerIndexLocationFlags flags = 0;
	GFlagsClass *type_class;
	GFlagsValue *value;

	type_class = g_type_class_ref (TRACKER_TYPE_INDEX_LOCATION_FLAGS);

	while (*flags_strv) {
		const gchar *flag_string = *flags_strv;

		value = g_flags_get_value_by_nick (type_class, flag_string);

		if (value != NULL) {
			flags |= value->value;
		}

		flags_strv ++;
	}

	g_type_class_unref (type_class);

	return flags;
}

static void
update_indexed_files (TrackerMinerFilesIndex *index)
{
	tracker_dbus_miner_files_proxy_set_indexed_locations (index->proxy_skeleton,
	                                                      (const gchar * const *) index->indexed_files->data);
}

static gboolean
tracker_miner_files_index_handle_index_location (TrackerDBusMinerFilesIndex *skeleton,
                                                 GDBusMethodInvocation      *invocation,
                                                 const gchar                *file_uri,
                                                 const gchar * const        *graphs,
                                                 const gchar * const        *flags,
                                                 TrackerMinerFilesIndex     *index)
{
	TrackerDBusRequest *request;
	TrackerIndexLocationFlags index_flags;
	g_autoptr (GFile) file = NULL;

	request = tracker_g_dbus_request_begin (invocation, "%s(uri:'%s')", __FUNCTION__, file_uri);

	file = g_file_new_for_uri (file_uri);

	if (!tracker_miner_files_peer_listener_is_file_watched (index->peer_listener, file)) {
		gchar *uri = g_strdup (file_uri);
		g_array_append_val (index->indexed_files, uri);
		update_indexed_files (index);
	}

	index_flags = parse_index_location_flags (flags);

	tracker_miner_files_peer_listener_add_watch (index->peer_listener,
	                                             g_dbus_method_invocation_get_sender (invocation),
	                                             file, graphs, index_flags);

	tracker_dbus_request_end (request, NULL);
	g_dbus_method_invocation_return_value (invocation, NULL);

	return TRUE;
}

static void
peer_listener_unwatch_file (TrackerMinerFilesPeerListener *listener,
                            GFile                         *file,
                            gpointer                       user_data)
{
	TrackerMinerFilesIndex *index = user_data;
	g_autofree char *uri = NULL;
	gint i;

	uri = g_file_get_uri (file);

	for (i = 0; i < index->indexed_files->len; i++) {
		const gchar *indexed_uri;

		indexed_uri = g_array_index (index->indexed_files, gchar*, i);

		if (g_strcmp0 (uri, indexed_uri) == 0) {
			g_array_remove_index (index->indexed_files, i);
			break;
		}
	}

	update_indexed_files (index);

	if (index->indexed_files->len == 0)
		g_signal_emit (index, signals[CLOSE], 0);
}

static void
peer_listener_graphs_changed (TrackerMinerFilesPeerListener *listener,
			      GStrv                          graphs,
			      gpointer                       user_data)
{
	TrackerMinerFilesIndex *index = user_data;

	g_strfreev (index->graphs);
	index->graphs = g_strdupv (graphs);

	tracker_dbus_miner_files_proxy_set_graphs (index->proxy_skeleton,
	                                           (const gchar * const *) index->graphs);
}

static void
string_clear (gpointer data)
{
	gchar **ptr = data;
	g_free (*ptr);
}

static void
tracker_miner_files_index_init (TrackerMinerFilesIndex *index)
{
	index->proxy_skeleton = tracker_dbus_miner_files_proxy_skeleton_new ();

	index->skeleton = tracker_dbus_miner_files_index_skeleton_new ();
	g_signal_connect (index->skeleton, "handle-index-location",
	                  G_CALLBACK (tracker_miner_files_index_handle_index_location),
	                  index);
	index->indexed_files = g_array_new (TRUE, TRUE, sizeof (gchar *));
	g_array_set_clear_func (index->indexed_files, string_clear);
}

static gboolean
tracker_miner_files_index_initable_init (GInitable     *initable,
                                         GCancellable  *cancellable,
                                         GError       **error)
{
	TrackerMinerFilesIndex *files_index =
		TRACKER_MINER_FILES_INDEX (initable);
	g_autofree char *full_path = NULL;

	files_index->d_connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, error);

	if (!files_index->d_connection)
		return FALSE;

	/* Register the service name for the miner */
	full_path = g_strconcat (TRACKER_MINER_DBUS_PATH_PREFIX, "Files/Index", NULL);

	g_debug ("Registering D-Bus object...");
	g_debug ("  Path:'%s'", full_path);
	g_debug ("  Object Type:'%s'", G_OBJECT_TYPE_NAME (files_index));

	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (files_index->skeleton),
	                                       files_index->d_connection,
	                                       full_path,
	                                       error))
		return FALSE;

	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (files_index->proxy_skeleton),
	                                       files_index->d_connection,
	                                       "/org/freedesktop/Tracker3/Miner/Files/Proxy",
	                                       error))
		return FALSE;

	files_index->full_path = full_path;

	files_index->peer_listener = tracker_miner_files_peer_listener_new (files_index->d_connection);
	g_signal_connect (files_index->peer_listener, "unwatch-file",
	                  G_CALLBACK (peer_listener_unwatch_file), files_index);
	g_signal_connect (files_index->peer_listener, "graphs-changed",
	                  G_CALLBACK (peer_listener_graphs_changed), files_index);

	return TRUE;
}

static void
tracker_miner_files_index_initable_iface_init (GInitableIface *iface)
{
	iface->init = tracker_miner_files_index_initable_init;
}

TrackerMinerFilesIndex *
tracker_miner_files_index_new (GError **error)
{
	return g_initable_new (TRACKER_TYPE_MINER_FILES_INDEX,
	                       NULL, error, NULL);
}
