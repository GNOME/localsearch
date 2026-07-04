/*
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>

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

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#include <glib.h>
#include <glib-unix.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include <pthread.h>

#ifdef __linux__
#include <linux/ioprio.h>
#include <sys/syscall.h>
#endif

#include <tracker-common.h>

#include "tracker-application.h"
#include "tracker-controller.h"
#include "tracker-files-interface.h"

#include <tinysparql.h>

static gboolean
signal_handler (gpointer user_data)
{
	GApplication *app = user_data;
	static gboolean in_loop = FALSE;

	/* Die if we get re-entrant signals handler calls */
	if (in_loop) {
		_exit (EXIT_FAILURE);
	}

	in_loop = TRUE;
	g_application_quit (app);

	return G_SOURCE_CONTINUE;
}

static void
initialize_signal_handler (GApplication *app)
{
#ifndef G_OS_WIN32
	g_unix_signal_add (SIGTERM, signal_handler, app);
	g_unix_signal_add (SIGINT, signal_handler, app);
#endif /* G_OS_WIN32 */
}

static void
initialize_priority_and_scheduling (void)
{
#ifdef __linux__
	struct sched_param sp = { 0, };
	int ioprio, ioclass, policy;

	/* Set CPU priority */
	TRACKER_NOTE (CONFIG, g_message ("Setting scheduler policy to SCHED_IDLE"));
	if (pthread_getschedparam (pthread_self (), &policy, &sp) >= 0) {
		/* Although pthread_setschedparam() should exist on any POSIX compliant OS,
		 * the SCHED_IDLE policy is Linux-specific. The POSIX standard only requires
		 * the existence of realtime and 'other' policies.
		 *
		 * We could set the priority to 0. On FreeBSD the default priority is already
		 * 0, and this may be true on other platforms, so we currently don't bother.
		 * See https://gitlab.gnome.org/GNOME/tracker-miners/merge_requests/140 for
		 * more discussion.
		 */
		if (pthread_setschedparam (pthread_self(), SCHED_IDLE, &sp) < 0)
			g_message ("Couldn't set idle scheduler policy: %m");
	}

	/* Set disk IO priority and scheduling */
	ioprio = 7; /* priority is ignored with idle class */
	ioclass = IOPRIO_CLASS_IDLE << IOPRIO_CLASS_SHIFT;
	if (syscall (SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0, ioprio | ioclass) < 0)
		g_message ("Couldn't set ioprio idle priority: %m");
#endif

	TRACKER_NOTE (CONFIG, g_message ("Setting priority nice level to 19"));
	if (nice (19) < 0)
		g_message ("Couldn't set nice value to 19: %m");
}

static void
raise_file_descriptor_limit (void)
{
	struct rlimit rl;

	if (getrlimit (RLIMIT_NOFILE, &rl) != 0)
		return;

	rl.rlim_cur = rl.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &rl) != 0)
		g_warning ("Failed to increase file descriptor limit: %m");
}

int
main (gint argc, gchar *argv[])
{
	g_autoptr (GApplication) app = NULL;
	g_autoptr (GError) error = NULL;
	int retval;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Set timezone info */
	tzset ();

	/* This makes sure we don't steal all the system's resources */
	initialize_priority_and_scheduling ();

	/* This makes it harder to run out of file descriptors while there
	 * are many concurrently running queries through the endpoint.
	 */
	raise_file_descriptor_limit ();

	app = tracker_application_new ();

	initialize_signal_handler (app);

	tracker_systemd_notify ("READY=1");

	retval = g_application_run (app, argc, argv);

	tracker_systemd_notify ("STOPPING=1");

	if (retval == EXIT_SUCCESS) {
		retval = tracker_application_exit_in_error (TRACKER_APPLICATION (app)) ?
			EXIT_FAILURE : EXIT_SUCCESS;
	}

	return retval;
}
