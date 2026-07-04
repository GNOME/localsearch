/*
 * Copyright (C) 2026 Red Hat Inc.
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
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#include "config-miners.h"

#include "tracker-module-manager.h"

#define EXTRACTOR_FUNCTION "tracker_extract_get_metadata"
#define INIT_FUNCTION      "tracker_extract_module_init"
#define SHUTDOWN_FUNCTION  "tracker_extract_module_shutdown"

typedef gboolean (* TrackerExtractInitFunc) (GError **error);
typedef void (* TrackerExtractShutdownFunc) (void);

typedef struct
{
	GModule *module;
	TrackerExtractInitFunc init_func;
	TrackerExtractShutdownFunc shutdown_func;
	TrackerExtractMetadataFunc extract_func;
} TrackerExtractModule;

struct _TrackerModuleManager
{
	GObject parent_instance;

	GHashTable *modules;
};

G_DEFINE_TYPE (TrackerModuleManager, tracker_module_manager, G_TYPE_OBJECT)

static TrackerExtractModule *
tracker_extract_module_new (const char  *module_path,
                            GError     **error)
{
	TrackerExtractModule *extract_module;
	GModule *module;
	TrackerExtractInitFunc init_func;
	TrackerExtractShutdownFunc shutdown_func;
	TrackerExtractMetadataFunc extract_func;

	/* Load the module */
	module = g_module_open (module_path, G_MODULE_BIND_LOCAL);

	if (!module) {
		g_set_error (error,
		             G_IO_ERROR, G_IO_ERROR_FAILED,
		             "Could not load module '%s': %s",
		             module_path,
		             g_module_error ());
		return NULL;
	}

	g_module_make_resident (module);

	if (!g_module_symbol (module, EXTRACTOR_FUNCTION, (gpointer *) &extract_func)) {
		g_set_error (error,
		             G_IO_ERROR, G_IO_ERROR_FAILED,
		             "Could not load module '%s': %s() was not found",
		             g_module_name (module), EXTRACTOR_FUNCTION);
		g_module_close (module);
		return NULL;
	}

	g_module_symbol (module, INIT_FUNCTION, (gpointer *) &init_func);
	g_module_symbol (module, SHUTDOWN_FUNCTION, (gpointer *) &shutdown_func);

	if (init_func && !init_func (error))
		return NULL;

	extract_module = g_new0 (TrackerExtractModule, 1);
	*extract_module = (TrackerExtractModule) {
		.module = module,
		.init_func = init_func,
		.shutdown_func = shutdown_func,
		.extract_func = extract_func,
	};

	return extract_module;
}

static void
tracker_extract_module_free (TrackerExtractModule *module)
{
	if (module->shutdown_func)
		module->shutdown_func ();

	g_module_close (module->module);
	g_free (module);
}

static gboolean
ensure_module (TrackerModuleManager  *manager,
               const char            *module_path,
               TrackerExtractModule **module_out,
               GError               **error)
{
	TrackerExtractModule *module;

	module = g_hash_table_lookup (manager->modules, module_path);

	if (!module) {
		module = tracker_extract_module_new (module_path, error);
		if (!module)
			return FALSE;

		g_hash_table_insert (manager->modules,
		                     g_strdup (module_path),
		                     module);
	}

	if (module_out)
		*module_out = module;

	return TRUE;
}

static void
tracker_module_manager_finalize (GObject *object)
{
	TrackerModuleManager *manager = TRACKER_MODULE_MANAGER (object);

	g_clear_pointer (&manager->modules, g_hash_table_unref);

	G_OBJECT_CLASS (tracker_module_manager_parent_class)->finalize (object);
}

static void
tracker_module_manager_class_init (TrackerModuleManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tracker_module_manager_finalize;
}

static void
tracker_module_manager_init (TrackerModuleManager *manager)
{
	manager->modules = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          g_free,
	                                          (GDestroyNotify) tracker_extract_module_free);
}

TrackerModuleManager *
tracker_module_manager_new (void)
{
	return g_object_new (TRACKER_TYPE_MODULE_MANAGER, NULL);
}

gboolean
tracker_module_manager_get_func (TrackerModuleManager        *manager,
                                 const char                  *module_path,
                                 TrackerExtractMetadataFunc  *func_out,
                                 GModule                    **module_out,
                                 GError                     **error)
{
	TrackerExtractModule *module;

	if (!ensure_module (manager, module_path, &module, error))
		return FALSE;

	if (func_out)
		*func_out = module->extract_func;
	if (module_out)
		*module_out = module->module;

	return TRUE;
}
