/*
 * Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
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

/**
 * SECTION: tracker-sparql-connection
 * @short_description: Connecting to the Store
 * @title: TrackerSparqlConnection
 * @stability: Stable
 * @include: tracker-sparql.h
 *
 * <para>
 * #TrackerSparqlConnection is an object which sets up connections to the
 * Tracker Store.
 * </para>
 */

// Convenience, hidden in the documentation
public const string TRACKER_DBUS_SERVICE = "org.freedesktop.Tracker1";
public const string TRACKER_DBUS_INTERFACE_RESOURCES = TRACKER_DBUS_SERVICE + ".Resources";
public const string TRACKER_DBUS_OBJECT_RESOURCES = "/org/freedesktop/Tracker1/Resources";
public const string TRACKER_DBUS_INTERFACE_STATISTICS = TRACKER_DBUS_SERVICE + ".Statistics";
public const string TRACKER_DBUS_OBJECT_STATISTICS = "/org/freedesktop/Tracker1/Statistics";
public const string TRACKER_DBUS_INTERFACE_STATUS = TRACKER_DBUS_SERVICE + ".Status";
public const string TRACKER_DBUS_OBJECT_STATUS = "/org/freedesktop/Tracker1/Status";
public const string TRACKER_DBUS_INTERFACE_STEROIDS = TRACKER_DBUS_SERVICE + ".Steroids";
public const string TRACKER_DBUS_OBJECT_STEROIDS = "/org/freedesktop/Tracker1/Steroids";


/**
 * TRACKER_SPARQL_ERROR:
 *
 * Error domain for Tracker Sparql. Errors in this domain will be from the
 * #TrackerSparqlError enumeration. See #GError for more information on error
 * domains.
 */

/**
 * TrackerSparqlError:
 * @TRACKER_SPARQL_ERROR_PARSE: Error parsing the SPARQL string.
 * @TRACKER_SPARQL_UNKNOWN_CLASS: Unknown class.
 * @TRACKER_SPARQL_UNKNOWN_PROPERTY: Unknown property.
 * @TRACKER_SPARQL_TYPE: Wrong type.
 * @TRACKER_SPARQL_INTERNAL: Internal error.
 * @TRACKER_SPARQL_UNSUPPORTED: Unsupported feature or method.
 *
 * Possible errors reported in the operations with the #TrackerSparqlConnection.
 */
[DBus (name = "org.freedesktop.DBus.GLib.UnmappedError.TrackerSparqlErrorQuark")]
public errordomain Tracker.Sparql.Error {
	PARSE,
	UNKNOWN_CLASS,
	UNKNOWN_PROPERTY,
	TYPE,
	CONSTRAINT,
	NO_SPACE,
	INTERNAL,
	UNSUPPORTED
}

/**
 * TrackerSparqlConnection:
 *
 * The <structname>TrackerSparqlConnection</structname> object represents a
 * connection with the Tracker Store.
 */
public abstract class Tracker.Sparql.Connection : Object {
	static bool direct_only;
	static weak Connection? singleton;
	static bool log_initialized;

	private static new Connection get_internal (bool is_direct_only = false, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		if (singleton != null) {
			assert (direct_only == is_direct_only);
			return singleton;
		}

		log_init ();

		if (cancellable != null && cancellable.is_cancelled ()) {
			throw new IOError.CANCELLED ("Operation was cancelled");
		}

		/* the True is to assert that direct only is required */
		Connection result = new Backend (is_direct_only);
		result.init ();

		if (cancellable != null && cancellable.is_cancelled ()) {
			throw new IOError.CANCELLED ("Operation was cancelled");
		}

		direct_only = is_direct_only;
		singleton = result;
		result.add_weak_pointer ((void**) (&singleton));
		return singleton;
	}

	private async static new Connection get_internal_async (bool is_direct_only = false, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		if (singleton != null) {
			assert (direct_only == is_direct_only);
			return singleton;
		}

		log_init ();

		if (cancellable != null && cancellable.is_cancelled ()) {
			throw new IOError.CANCELLED ("Operation was cancelled");
		}

		/* the True is to assert that direct only is required */
		Connection result = new Backend (is_direct_only);
		yield result.init_async ();

		if (cancellable != null && cancellable.is_cancelled ()) {
			throw new IOError.CANCELLED ("Operation was cancelled");
		}

		direct_only = is_direct_only;
		singleton = result;
		result.add_weak_pointer ((void**) (&singleton));
		return singleton;
	}

	/**
	 * tracker_sparql_connection_get_direct_async:
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * See tracker_sparql_connection_get().
	 *
	 * Returns: a new #TrackerSparqlConnection. Call g_object_unref() on the
	 * object when no longer used.
	 */
	public async static new Connection get_async (Cancellable? cancellable = null) throws Sparql.Error, IOError {
		return yield get_internal_async (false, cancellable);
	}

	/**
	 * tracker_sparql_connection_get:
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * Returns a new #TrackerSparqlConnection, which will use the best method
	 * available to connect to the Tracker Store (direct-access for Read-Only
	 * queries, and D-Bus otherwise).
	 *
	 * There are 2 environment variables which can be used to control which
	 * backends are used to set up the connection. If no environment variables are
	 * provided, then both backends are loaded and chosen based on their merits.
	 *
	 * The TRACKER_SPARQL_BACKEND environment variable also allows the caller to
	 * switch between "auto" (the default), "direct" (for direct access) and
	 * "bus" for D-Bus backends. If you force a backend which does not support
	 * what you're doing (for example, using the "direct" backend for a SPARQL
	 * update) then you will see critical warnings in your code.
	 *
	 * Returns: a new #TrackerSparqlConnection. Call g_object_unref() on the
	 * object when no longer used.
	 */
	public static new Connection get (Cancellable? cancellable = null) throws Sparql.Error, IOError {
		return get_internal (false, cancellable);
	}

	/**
	 * tracker_sparql_connection_get_direct_async:
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * See tracker_sparql_connection_get_direct().
	 *
	 * Returns: a new #TrackerSparqlConnection. Call g_object_unref() on the
	 * object when no longer used.
	 */
	public async static Connection get_direct_async (Cancellable? cancellable = null) throws Sparql.Error, IOError {
		return yield get_internal_async (true, cancellable);
	}

	/**
	 * tracker_sparql_connection_get_direct:
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * Returns a new #TrackerSparqlConnection, which uses direct-access method
	 * to connect to the Tracker Store. Note that this connection will only be
	 * able to perform Read-Only queries in the store.
	 *
	 * If the TRACKER_SPARQL_BACKEND environment variable is set, it may
	 * override the choice to use a direct access connection here, for more
	 * details, see tracker_sparql_connection_get().
	 *
	 * Returns: a new #TrackerSparqlConnection. Call g_object_unref() on the
	 * object when no longer used.
	 */
	public static new Connection get_direct (Cancellable? cancellable = null) throws Sparql.Error, IOError {
		return get_internal (true, cancellable);
	}

	private static void log_init () {
		if (log_initialized) {
			return;
		}

		log_initialized = true;

		// Avoid debug messages
		int verbosity = 0;
		string env_verbosity = Environment.get_variable ("TRACKER_VERBOSITY");
		if (env_verbosity != null)
			verbosity = env_verbosity.to_int ();

		LogLevelFlags remove_levels = 0;

		switch (verbosity) {
		// Log level 3: EVERYTHING
		case 3:
			break;

		// Log level 2: CRITICAL/ERROR/WARNING/INFO/MESSAGE only
		case 2:
			remove_levels = LogLevelFlags.LEVEL_DEBUG;
			break;

		// Log level 1: CRITICAL/ERROR/WARNING/INFO only
		case 1:
			remove_levels = LogLevelFlags.LEVEL_DEBUG |
			              LogLevelFlags.LEVEL_MESSAGE;
			break;

		// Log level 0: CRITICAL/ERROR/WARNING only (default)
		default:
		case 0:
			remove_levels = LogLevelFlags.LEVEL_DEBUG |
			              LogLevelFlags.LEVEL_MESSAGE |
			              LogLevelFlags.LEVEL_INFO;
			break;
		}

		if (remove_levels != 0) {
			GLib.Log.set_handler ("Tracker", remove_levels, remove_log_handler);
		}
	}

	private static void remove_log_handler (string? log_domain, LogLevelFlags log_level, string message) {
		/* do nothing */
	}

	public virtual void init () throws Sparql.Error {
		warning ("Interface 'init' not implemented");
	}

	public async virtual void init_async () throws Sparql.Error {
		warning ("Interface 'init_async' not implemented");
	}

	/**
	 * tracker_sparql_connection_query:
	 * @self: a #TrackerSparqlConnection
	 * @sparql: string containing the SPARQL query
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * Executes a SPARQL query on the store. The API call is completely
	 * synchronous, so it may block.
	 *
	 * Returns: a #TrackerSparqlCursor if results were found, #NULL otherwise.
	 * On error, #NULL is returned and the @error is set accordingly.
	 * Call g_object_unref() on the returned cursor when no longer needed.
	 */
	public abstract Cursor query (string sparql, Cancellable? cancellable = null) throws Sparql.Error, IOError;

	/**
	 * tracker_sparql_connection_query_async:
	 * @self: a #TrackerSparqlConnection
	 * @sparql: string containing the SPARQL query
	 * @_callback_: user-defined #GAsyncReadyCallback to be called when
	 *              asynchronous operation is finished.
	 * @_user_data_: user-defined data to be passed to @_callback_
	 * @cancellable: a #GCancellable used to cancel the operation
	 *
	 * Executes asynchronously a SPARQL query on the store.
	 */

	/**
	 * tracker_sparql_connection_query_finish:
	 * @self: a #TrackerSparqlConnection
	 * @_res_: a #GAsyncResult with the result of the operation
	 * @error: #GError for error reporting.
	 *
	 * Finishes the asynchronous SPARQL query operation.
	 *
	 * Returns: a #TrackerSparqlCursor if results were found, #NULL otherwise.
	 * On error, #NULL is returned and the @error is set accordingly.
	 * Call g_object_unref() on the returned cursor when no longer needed.
	 */
	public async abstract Cursor query_async (string sparql, Cancellable? cancellable = null) throws Sparql.Error, IOError;

	/**
	 * tracker_sparql_connection_update:
	 * @self: a #TrackerSparqlConnection
	 * @sparql: string containing the SPARQL update query
	 * @priority: the priority for the operation
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * Executes a SPARQL update on the store. The API call is completely
	 * synchronous, so it may block.
	 */
	public virtual void update (string sparql, int priority = GLib.Priority.DEFAULT, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'update' not implemented");
	}

	/**
	 * tracker_sparql_connection_update_async:
	 * @self: a #TrackerSparqlConnection
	 * @sparql: string containing the SPARQL update query
	 * @priority: the priority for the asynchronous operation
	 * @_callback_: user-defined #GAsyncReadyCallback to be called when
	 *              asynchronous operation is finished.
	 * @_user_data_: user-defined data to be passed to @_callback_
	 * @cancellable: a #GCancellable used to cancel the operation
	 *
	 * Executes asynchronously a SPARQL update on the store.
	 */

	/**
	 * tracker_sparql_connection_update_finish:
	 * @self: a #TrackerSparqlConnection
	 * @_res_: a #GAsyncResult with the result of the operation
	 * @error: #GError for error reporting.
	 *
	 * Finishes the asynchronous SPARQL update operation.
	 */
	public async virtual void update_async (string sparql, int priority = GLib.Priority.DEFAULT, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'update_async' not implemented");
	}

	/**
	 * tracker_sparql_connection_update_blank:
	 * @self: a #TrackerSparqlConnection
	 * @sparql: string containing the SPARQL update query
	 * @priority: the priority for the operation
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * Executes a SPARQL update on the store, and returns the URNs of the
	 * generated nodes, if any. The API call is completely synchronous, so it
	 * may block.
	 *
	 * Returns: a #GVariant with the generated URNs, which should be freed with
	 * g_variant_unref() when no longer used.
	 */
	public virtual GLib.Variant? update_blank (string sparql, int priority = GLib.Priority.DEFAULT, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'update_blank' not implemented");
		return null;
	}

	/**
	 * tracker_sparql_connection_update_blank_async:
	 * @self: a #TrackerSparqlConnection
	 * @sparql: string containing the SPARQL update query
	 * @priority: the priority for the asynchronous operation
	 * @_callback_: user-defined #GAsyncReadyCallback to be called when
	 *              asynchronous operation is finished.
	 * @_user_data_: user-defined data to be passed to @_callback_
	 * @cancellable: a #GCancellable used to cancel the operation
	 *
	 * Executes asynchronously a SPARQL update on the store.
	 */

	/**
	 * tracker_sparql_connection_update_blank_finish:
	 * @self: a #TrackerSparqlConnection
	 * @_res_: a #GAsyncResult with the result of the operation
	 * @error: #GError for error reporting.
	 *
	 * Finishes the asynchronous SPARQL update operation, and returns
	 * the URNs of the generated nodes, if any.
	 *
	 * Returns: a #GVariant with the generated URNs, which should be freed with
	 * g_variant_unref() when no longer used.
	 */
	public async virtual GLib.Variant? update_blank_async (string sparql, int priority = GLib.Priority.DEFAULT, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'update_blank_async' not implemented");
		return null;
	}

	/**
	 * tracker_sparql_connection_load:
	 * @self: a #TrackerSparqlConnection
	 * @file: a #GFile
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * Loads a Turtle file (TTL) into the store. The API call is completely
	 * synchronous, so it may block.
	 */
	public virtual void load (File file, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'load' not implemented");
	}

	/**
	 * tracker_sparql_connection_load_async:
	 * @self: a #TrackerSparqlConnection
	 * @file: a #GFile
	 * @_callback_: user-defined #GAsyncReadyCallback to be called when
	 *              asynchronous operation is finished.
	 * @_user_data_: user-defined data to be passed to @_callback_
	 * @cancellable: a #GCancellable used to cancel the operation
	 *
	 * Loads, asynchronously, a Turtle file (TTL) into the store.
	 */

	/**
	 * tracker_sparql_connection_load_finish:
	 * @self: a #TrackerSparqlConnection
	 * @_res_: a #GAsyncResult with the result of the operation
	 * @error: #GError for error reporting.
	 *
	 * Finishes the asynchronous load of the Turtle file.
	 */
	public async virtual void load_async (File file, Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'load_async' not implemented");
	}

	/**
	 * tracker_sparql_connection_statistics:
	 * @self: a #TrackerSparqlConnection
	 * @cancellable: a #GCancellable used to cancel the operation
	 * @error: #GError for error reporting.
	 *
	 * Retrieves the statistics from the Store. The API call is completely
	 * synchronous, so it may block.
	 *
	 * Returns: a #TrackerSparqlCursor to iterate the reply if successful, #NULL
	 * on error. Call g_object_unref() on the returned cursor when no longer
	 * needed.
	 */
	public virtual Cursor? statistics (Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'statistics' not implemented");
		return null;
	}

	/**
	 * tracker_sparql_connection_statistics_async:
	 * @self: a #TrackerSparqlConnection
	 * @_callback_: user-defined #GAsyncReadyCallback to be called when
	 *              asynchronous operation is finished.
	 * @_user_data_: user-defined data to be passed to @_callback_
	 * @cancellable: a #GCancellable used to cancel the operation
	 *
	 * Retrieves, asynchronously, the statistics from the Store.
	 */

	/**
	 * tracker_sparql_connection_statistics_finish:
	 * @self: a #TrackerSparqlConnection
	 * @_res_: a #GAsyncResult with the result of the operation
	 * @error: #GError for error reporting.
	 *
	 * Finishes the asynchronous retrieval of statistics from the Store.
	 *
	 * Returns: a #TrackerSparqlCursor to iterate the reply if successful, #NULL
	 * on error. Call g_object_unref() on the returned cursor when no longer
	 * needed.
	 */
	public async virtual Cursor? statistics_async (Cancellable? cancellable = null) throws Sparql.Error, IOError {
		warning ("Interface 'statistics_async' not implemented");
		return null;
	}
}
