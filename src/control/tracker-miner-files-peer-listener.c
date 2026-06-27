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

/* TrackerMinerFilesPeerListener is a helper object to keep track
 * of the DBus callers that request directories to be indexed. The
 * ::unwatch-file will be emitted as soon as there's no further
 * requestors on a directory, be it due to disconnections, or due
 * to other reasons (tracker_indexing_tree_remove() being called
 * externally, eg. when the directory being monitored is unmounted).
 */

#include "tracker-miner-files-peer-listener.h"

enum {
	PROP_0,
	PROP_CONNECTION,
	N_PROPS,
};

static GParamSpec *props[N_PROPS] = { 0, };

enum {
	WATCH_FILE,
	UNWATCH_FILE,
	GRAPHS_CHANGED,
	N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

typedef struct {
	gchar *dbus_name;
	GPtrArray *graphs;
	GPtrArray *files; /* Array of GFiles, actually owned by FilePeersData */
	guint watch_id;
} PeerFilesData;

typedef struct {
	GFile *file;
	GPtrArray *peers; /* Array of dbus names, actually owned by PeerFilesData */
} FilePeersData;

struct _TrackerMinerFilesPeerListener {
	GObject parent_instance;

	GDBusConnection *d_connection;
	GHashTable *peer_files; /* dbus name -> PeerFilesData */
	GHashTable *file_peers; /* GFile -> FilePeersData */
	GStrv graphs;
};

G_DEFINE_TYPE (TrackerMinerFilesPeerListener,
               tracker_miner_files_peer_listener,
               G_TYPE_OBJECT)

static void
on_app_disappeared_cb (GDBusConnection *conn,
                       const gchar     *dbus_name,
                       gpointer         user_data)
{
	TrackerMinerFilesPeerListener *listener = user_data;

	tracker_miner_files_peer_listener_remove_dbus_name (listener, dbus_name);
}

static PeerFilesData *
peer_files_data_new (const gchar                   *dbus_name,
                     TrackerMinerFilesPeerListener *listener)
{
	PeerFilesData *data;

	data = g_slice_new0 (PeerFilesData);
	data->dbus_name = g_strdup (dbus_name);
	data->files = g_ptr_array_new ();
	data->graphs = g_ptr_array_new_with_free_func (g_free);
	data->watch_id = g_bus_watch_name_on_connection (listener->d_connection,
	                                                 dbus_name, 0, NULL,
	                                                 on_app_disappeared_cb,
	                                                 listener, NULL);
	return data;
}

static void
peer_files_data_free (PeerFilesData *data)
{
	g_bus_unwatch_name (data->watch_id);
	g_ptr_array_unref (data->files);
	g_ptr_array_unref (data->graphs);
	g_free (data->dbus_name);
	g_slice_free (PeerFilesData, data);
}

static void
peer_files_data_add_file (PeerFilesData *data,
                          GFile         *file)
{
	g_ptr_array_add (data->files, file);
}

static void
peer_files_data_add_graphs (PeerFilesData       *data,
                            const gchar * const *graphs)
{
	gint i;

	for (i = 0; graphs[i] != NULL; i++) {
		g_ptr_array_add (data->graphs, g_strdup (graphs[i]));
	}
}

static FilePeersData *
file_peers_data_new (GFile *file)
{
	FilePeersData *data;

	g_return_val_if_fail (G_IS_FILE (file), NULL);

	data = g_slice_new0 (FilePeersData);
	data->file = g_object_ref (file);
	data->peers = g_ptr_array_new ();

	return data;
}

static void
file_peers_data_free (FilePeersData *data)
{
	g_object_unref (data->file);
	g_ptr_array_unref (data->peers);
	g_slice_free (FilePeersData, data);
}

static void
file_peers_data_add_dbus_name (FilePeersData *data,
                               gchar         *dbus_name)
{
	g_ptr_array_add (data->peers, dbus_name);
}

static void
file_peers_data_remove_dbus_name (FilePeersData *data,
                                  gchar         *dbus_name)
{
	gint i;

	for (i = 0; i < data->peers->len; i++) {
		if (dbus_name != g_ptr_array_index (data->peers, i))
			continue;

		g_ptr_array_remove_index (data->peers, i);
		break;
	}
}

static void
tracker_miner_files_peer_listener_finalize (GObject *object)
{
	TrackerMinerFilesPeerListener *listener =
		TRACKER_MINER_FILES_PEER_LISTENER (object);

	g_hash_table_destroy (listener->peer_files);
	g_hash_table_destroy (listener->file_peers);
	g_object_unref (listener->d_connection);

	G_OBJECT_CLASS (tracker_miner_files_peer_listener_parent_class)->finalize (object);
}

static void
tracker_miner_files_peer_listener_set_property (GObject      *object,
                                                guint         prop_id,
                                                const GValue *value,
                                                GParamSpec   *pspec)
{
	TrackerMinerFilesPeerListener *listener;

	listener = TRACKER_MINER_FILES_PEER_LISTENER (object);

	switch (prop_id) {
	case PROP_CONNECTION:
		listener->d_connection = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
tracker_miner_files_peer_listener_class_init (TrackerMinerFilesPeerListenerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tracker_miner_files_peer_listener_finalize;
	object_class->set_property = tracker_miner_files_peer_listener_set_property;

	props[PROP_CONNECTION] =
		g_param_spec_object ("connection", NULL, NULL,
		                     G_TYPE_DBUS_CONNECTION,
		                     G_PARAM_WRITABLE |
		                     G_PARAM_CONSTRUCT_ONLY |
		                     G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPS, props);

	signals[WATCH_FILE] =
		g_signal_new ("watch-file",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST, 0,
		              NULL, NULL, NULL,
		              G_TYPE_NONE, 1, G_TYPE_FILE);
	signals[UNWATCH_FILE] =
		g_signal_new ("unwatch-file",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST, 0,
		              NULL, NULL, NULL,
		              G_TYPE_NONE, 1, G_TYPE_FILE);
	signals[GRAPHS_CHANGED] =
		g_signal_new ("graphs-changed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST, 0,
		              NULL, NULL, NULL,
		              G_TYPE_NONE, 1, G_TYPE_STRV);
}

static void
tracker_miner_files_peer_listener_init (TrackerMinerFilesPeerListener *listener)
{
	listener->peer_files =
		g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
		                       (GDestroyNotify) peer_files_data_free);
	listener->file_peers =
		g_hash_table_new_full (g_file_hash,
		                       (GEqualFunc) g_file_equal, NULL,
		                       (GDestroyNotify) file_peers_data_free);
}

TrackerMinerFilesPeerListener *
tracker_miner_files_peer_listener_new (GDBusConnection *connection)
{
	return g_object_new (TRACKER_TYPE_MINER_FILES_PEER_LISTENER,
	                     "connection", connection,
	                     NULL);
}

static void
unwatch_file (TrackerMinerFilesPeerListener *listener,
              GFile                         *file)
{
	g_autoptr (GFile) f = NULL;

	f = g_object_ref (file);
	g_hash_table_remove (listener->file_peers, f);
	g_signal_emit (listener, signals[UNWATCH_FILE], 0, f);
}

static void
update_graphs (TrackerMinerFilesPeerListener *listener)
{
	PeerFilesData *peer_data;
	GHashTableIter iter;
	const gchar *graph;
	GHashTable *ht;
	GArray *array;
	gint i;

	ht = g_hash_table_new (g_str_hash, g_str_equal);
	array = g_array_new (TRUE, TRUE, sizeof (char *));
	g_hash_table_iter_init (&iter, listener->peer_files);

	while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &peer_data)) {
		for (i = 0; i < peer_data->graphs->len; i++) {
			g_hash_table_add (ht, g_ptr_array_index (peer_data->graphs, i));
		}
	}

	g_hash_table_iter_init (&iter, ht);

	while (g_hash_table_iter_next (&iter, (gpointer *) &graph, NULL)) {
		g_array_append_val (array, graph);
	}

	g_signal_emit (listener, signals[GRAPHS_CHANGED], 0, array->data);
	g_hash_table_unref (ht);
	g_array_free (array, TRUE);
}

void
tracker_miner_files_peer_listener_add_watch (TrackerMinerFilesPeerListener *listener,
                                             const gchar                   *dbus_name,
                                             GFile                         *file,
                                             const gchar * const           *graphs,
                                             TrackerIndexLocationFlags      flags)
{
	PeerFilesData *peer_data;
	FilePeersData *file_data;

	g_return_if_fail (TRACKER_IS_MINER_FILES_PEER_LISTENER (listener));
	g_return_if_fail (G_IS_FILE (file));
	g_return_if_fail (dbus_name != NULL);

	peer_data = g_hash_table_lookup (listener->peer_files, dbus_name);
	file_data = g_hash_table_lookup (listener->file_peers, file);

	if (!peer_data) {
		peer_data = peer_files_data_new (dbus_name, listener);
		g_hash_table_insert (listener->peer_files,
		                     peer_data->dbus_name, peer_data);
	}

	if (!file_data) {
		gchar *uri;

		file_data = file_peers_data_new (file);
		g_hash_table_insert (listener->file_peers,
		                     file_data->file, file_data);
		g_signal_emit (listener, signals[WATCH_FILE], 0, file_data->file);

		uri = g_file_get_uri (file);
		g_debug ("Client '%s' requests watch on '%s'", dbus_name, uri);
		g_free (uri);
	}

	peer_files_data_add_file (peer_data, file_data->file);
	peer_files_data_add_graphs (peer_data, graphs);
	file_peers_data_add_dbus_name (file_data, peer_data->dbus_name);

	update_graphs (listener);
}

void
tracker_miner_files_peer_listener_remove_dbus_name (TrackerMinerFilesPeerListener *listener,
                                                    const gchar                   *dbus_name)
{
	PeerFilesData *peer_data;
	FilePeersData *file_data;
	GFile *file;
	gint i;

	g_return_if_fail (TRACKER_IS_MINER_FILES_PEER_LISTENER (listener));
	g_return_if_fail (dbus_name != NULL);

	peer_data = g_hash_table_lookup (listener->peer_files, dbus_name);

	if (!peer_data)
		return;

	g_debug ("Removing all watches from client '%s'", dbus_name);

	for (i = 0; i < peer_data->files->len; i++) {
		file = g_ptr_array_index (peer_data->files, i);
		file_data = g_hash_table_lookup (listener->file_peers, file);

		if (!file_data)
			continue;

		file_peers_data_remove_dbus_name (file_data, peer_data->dbus_name);

		if (file_data->peers->len == 0)
			unwatch_file (listener, file_data->file);
	}

	g_hash_table_remove (listener->peer_files, dbus_name);
	update_graphs (listener);
}

gboolean
tracker_miner_files_peer_listener_is_file_watched (TrackerMinerFilesPeerListener *listener,
                                                   GFile                         *file)
{
	g_return_val_if_fail (TRACKER_IS_MINER_FILES_PEER_LISTENER (listener), FALSE);
	g_return_val_if_fail (G_IS_FILE (file), FALSE);

	return g_hash_table_lookup (listener->file_peers, file) != NULL;
}
