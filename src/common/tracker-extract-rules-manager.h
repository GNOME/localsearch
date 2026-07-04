/*
 * Copyright (C) 2011, Nokia <ivan.frade@nokia.com>
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

#pragma once

#include <glib.h>

gboolean tracker_extract_rules_manager_init (void);

GStrv tracker_extract_rules_manager_get_rdf_types (const char *mimetype);

const char * tracker_extract_rules_manager_get_graph (const char *mimetype);

const char * tracker_extract_rules_manager_get_hash  (const char *mimetype);

gboolean tracker_extract_rules_manager_check_fallback_rdf_type (const char *mimetype,
                                                                const char *rdf_type);

const char * tracker_extract_rules_manager_get_module (const char *mimetype);

GList * tracker_extract_rules_manager_get_matching_rules (const char *mimetype);
