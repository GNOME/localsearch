/*
 * Copyright (C) 2006, Jamie McCracken <jamiemcc@gnome.org>
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

#include <upower.h>

#include "tracker-power.h"

struct _TrackerPower {
	GObject parent_instance;

	UpClient  *client;
#ifndef HAVE_UP_CLIENT_GET_ON_LOW_BATTERY
	UpDevice  *composite_device;
#endif
	gboolean   on_battery;
	gboolean   on_low_battery;
};

static void     tracker_power_initable_iface_init (GInitableIface *iface);
static void     tracker_power_finalize            (GObject         *object);
static void     tracker_power_get_property        (GObject         *object,
                                                   guint            param_id,
                                                   GValue                  *value,
                                                   GParamSpec      *pspec);
#ifdef HAVE_UP_CLIENT_GET_ON_LOW_BATTERY
static void     tracker_power_client_changed_cb   (UpClient        *client,
                                                   TrackerPower    *power);
#endif /* HAVE_UP_CLIENT_GET_ON_LOW_BATTERY */

enum {
	PROP_0,
	PROP_ON_BATTERY,
	PROP_ON_LOW_BATTERY,
	N_PROPS,
};

static GParamSpec *props[N_PROPS] = { 0, };

G_DEFINE_TYPE_WITH_CODE (TrackerPower, tracker_power, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, tracker_power_initable_iface_init));

static void
tracker_power_class_init (TrackerPowerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize     = tracker_power_finalize;
	object_class->get_property = tracker_power_get_property;

	props[PROP_ON_BATTERY] =
		g_param_spec_boolean ("on-battery", NULL, NULL,
		                      FALSE,
		                      G_PARAM_READABLE |
		                      G_PARAM_STATIC_STRINGS);
	props[PROP_ON_LOW_BATTERY] =
		g_param_spec_boolean ("on-low-battery", NULL, NULL,
		                      FALSE,
		                      G_PARAM_READABLE |
		                      G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPS, props);
}

#ifndef HAVE_UP_CLIENT_GET_ON_LOW_BATTERY
static void
on_on_battery_changed (UpClient     *client,
                       GParamSpec   *pspec,
                       TrackerPower *power)
{
	gboolean on_battery;

	on_battery = up_client_get_on_battery (power->client);
	if (on_battery != power->on_battery) {
		power->on_battery = on_battery;
		g_object_notify_by_pspec (G_OBJECT (power),
		                          props[PROP_ON_BATTERY]);
	}
}

static void
on_warning_level_changed (UpDevice     *device,
                          GParamSpec   *pspec,
                          TrackerPower *power)
{
	UpDeviceLevel warning_level;
	gboolean on_low_battery;

	g_object_get (power->composite_device, "warning-level", &warning_level, NULL);
	on_low_battery = warning_level >= UP_DEVICE_LEVEL_LOW;
	if (on_low_battery != power->on_low_battery) {
		power->on_low_battery = on_low_battery;
		g_object_notify_by_pspec (G_OBJECT (power),
		                          props[PROP_ON_LOW_BATTERY]);
	}
}
#endif /* !HAVE_UP_CLIENT_GET_ON_LOW_BATTERY */

static void
tracker_power_init (TrackerPower *power)
{
}
static gboolean
tracker_power_initable_init (GInitable     *initable,
                             GCancellable  *cancellable,
                             GError       **error)
{
	TrackerPower *power = TRACKER_POWER (initable);

	/* connect to a UPower instance */
	power->client = up_client_new ();

	if (power->client == NULL) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_DBUS_ERROR, "Unable to connect to UPower");
		return FALSE;
	}

#ifdef HAVE_UP_CLIENT_GET_ON_LOW_BATTERY
	g_signal_connect (power->client, "changed",
	                  G_CALLBACK (tracker_power_client_changed_cb), power);
	tracker_power_client_changed_cb (power->client, power);
#else
	g_signal_connect (power->client, "notify::on-battery",
	                  G_CALLBACK (on_on_battery_changed), power);
	on_on_battery_changed (power->client, NULL, power);
	power->composite_device = up_client_get_display_device (power->client);
	g_signal_connect (power->composite_device, "notify::warning-level",
		              G_CALLBACK (on_warning_level_changed), power);
	on_warning_level_changed (power->composite_device, NULL, power);
#endif /* HAVE_UP_CLIENT_GET_ON_LOW_BATTERY */

	return TRUE;
}

static void
tracker_power_finalize (GObject *object)
{
	TrackerPower *power = TRACKER_POWER (object);

#ifndef HAVE_UP_CLIENT_GET_ON_LOW_BATTERY
	g_clear_object (&power->composite_device);
#endif /* HAVE_UP_CLIENT_GET_ON_LOW_BATTERY */

	g_clear_object (&power->client);

	(G_OBJECT_CLASS (tracker_power_parent_class)->finalize) (object);
}

static void
tracker_power_get_property (GObject    *object,
                            guint       param_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	TrackerPower *power = TRACKER_POWER (object);

	switch (param_id) {
	case PROP_ON_BATTERY:
		g_value_set_boolean (value, power->on_battery);
		break;
	case PROP_ON_LOW_BATTERY:
		g_value_set_boolean (value, power->on_low_battery);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

#ifdef HAVE_UP_CLIENT_GET_ON_LOW_BATTERY
static void
tracker_power_client_changed_cb (UpClient     *client,
                                 TrackerPower *power)
{
	gboolean on_battery;
	gboolean on_low_battery;

	/* get the on-battery state */
	on_battery = up_client_get_on_battery (power->client);
	if (on_battery != power->on_battery) {
		power->on_battery = on_battery;
		g_object_notify_by_pspec (G_OBJECT (power),
		                          props[PROP_ON_BATTERY])
	}

	/* get the on-low-battery state */
	on_low_battery = up_client_get_on_low_battery (power->client);
	if (on_low_battery != power->on_low_battery) {
		power->on_low_battery = on_low_battery;
		g_object_notify_by_pspec (G_OBJECT (power),
		                          props[PROP_ON_LOW_BATTERY])
	}
}
#endif /* HAVE_UP_CLIENT_GET_ON_LOW_BATTERY */

static void
tracker_power_initable_iface_init (GInitableIface *iface)
{
	iface->init = tracker_power_initable_init;
}

/**
 * tracker_power_new:
 *
 * Creates a new instance of #TrackerPower.
 *
 * Returns: The newly created #TrackerPower, or %NULL if there was an error.
 **/
TrackerPower *
tracker_power_new ()
{
	g_autoptr (GError) error = NULL;
	TrackerPower *object;

	object = g_initable_new (TRACKER_TYPE_POWER, NULL, &error, NULL);

	if (error) {
		g_warning ("%s", error->message);
	}

	return object;
}

/**
 * tracker_power_get_on_battery:
 * @power: A #TrackerPower.
 *
 * Returns whether the computer battery (if any) is currently in use.
 *
 * Returns: #TRUE if the computer is running on battery power.
 **/
gboolean
tracker_power_get_on_battery (TrackerPower *power)
{
	gboolean on_battery;

	g_return_val_if_fail (TRACKER_IS_POWER (power), TRUE);

	g_object_get (G_OBJECT (power), "on-battery", &on_battery, NULL);

	return on_battery;
}

/**
 * tracker_power_get_on_low_battery:
 * @power: A #TrackerPower
 *
 * Returns whether the computer has batteries.
 *
 * Returns: #TRUE if the computer has batteries available.
 **/
gboolean
tracker_power_get_on_low_battery (TrackerPower *power)
{
	gboolean on_low_battery;

	g_return_val_if_fail (TRACKER_IS_POWER (power), TRUE);

	g_object_get (G_OBJECT (power), "on-low-battery", &on_low_battery, NULL);

	return on_low_battery;
}
