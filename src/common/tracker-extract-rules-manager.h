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

#include <gio/gio.h>

#define TRACKER_TYPE_EXTRACT_RULES_MANAGER (tracker_extract_rules_manager_get_type ())
G_DECLARE_FINAL_TYPE (TrackerExtractRulesManager,
                      tracker_extract_rules_manager,
                      TRACKER, EXTRACT_RULES_MANAGER,
                      GObject);

TrackerExtractRulesManager * tracker_extract_rules_manager_new (GError **error);

GStrv tracker_extract_rules_manager_get_rdf_types (TrackerExtractRulesManager *manager,
                                                   const char                 *mimetype);

const char * tracker_extract_rules_manager_get_graph (TrackerExtractRulesManager *manager,
                                                      const char                 *mimetype);

const char * tracker_extract_rules_manager_get_hash  (TrackerExtractRulesManager *manager,
                                                      const char                 *mimetype);

gboolean tracker_extract_rules_manager_check_fallback_rdf_type (TrackerExtractRulesManager *manager,
                                                                const char                 *mimetype,
                                                                const char                 *rdf_type);

const char * tracker_extract_rules_manager_get_module (TrackerExtractRulesManager *manager,
                                                       const char                 *mimetype);
