/*
 * Copyright (C) 2019 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "fu-io-channel.h"
#include "fu-thelio-io-device.h"

struct _FuThelioIoDevice {
	FuUsbDevice			 parent_instance;
};

G_DEFINE_TYPE (FuThelioIoDevice, fu_thelio_io_device, FU_TYPE_USB_DEVICE)

static gboolean
fu_thelio_io_device_probe (FuDevice *device, GError **error)
{
	gssize read_size;
	g_autofree gchar *fn = NULL;
	g_autoptr(GUdevDevice) udev_device = NULL;
	g_autoptr(GFile) file = NULL;
	g_autoptr(GInputStream) istr = NULL;
	guint8 buf[4096] = { 0 };

	fu_device_add_instance_id (device, "USB\\VID_03EB&PID_2FF4");

	/* convert GUsbDevice to GUdevDevice */
	udev_device = fu_usb_device_find_udev_device (FU_USB_DEVICE (device), error);
	if (udev_device == NULL)
		return FALSE;

	fn = g_build_filename (g_udev_device_get_sysfs_path (udev_device), "revision", NULL);
	file = g_file_new_for_path (fn);
	istr = G_INPUT_STREAM (g_file_read (file, NULL, error));
	if (istr == NULL)
		return FALSE;

	read_size = g_input_stream_read (istr, buf, sizeof(buf) - 1, NULL, error);
	if (read_size < 0) {
		fu_device_set_version (device, "0.0.0", FWUPD_VERSION_FORMAT_TRIPLET);
	} else {
		buf[read_size] = 0;
		fu_device_set_version (device, (const gchar *) buf, FWUPD_VERSION_FORMAT_TRIPLET);
	}

	return TRUE;
}

static gboolean
fu_thelio_io_device_detach (FuDevice *device, GError **error)
{
	g_autofree gchar *fn = NULL;
	g_autoptr(FuIOChannel) io_channel = NULL;
	g_autoptr(GUdevDevice) udev_device = NULL;
	const guint8 buf[] = { '1', '\n' };

	/* convert GUsbDevice to GUdevDevice */
	udev_device = fu_usb_device_find_udev_device (FU_USB_DEVICE (device), error);
	if (udev_device == NULL)
		return FALSE;
	fu_device_set_status (device, FWUPD_STATUS_DEVICE_RESTART);
	fn = g_build_filename (g_udev_device_get_sysfs_path (udev_device), "bootloader", NULL);
	io_channel = fu_io_channel_new_file (fn, error);
	if (io_channel == NULL)
		return FALSE;
	return fu_io_channel_write_raw (io_channel, buf, sizeof(buf),
					500, FU_IO_CHANNEL_FLAG_SINGLE_SHOT, error);
}

static void
fu_thelio_io_device_init (FuThelioIoDevice *self)
{
	fu_device_add_flag (FU_DEVICE (self), FWUPD_DEVICE_FLAG_UPDATABLE);
	fu_device_set_remove_delay (FU_DEVICE (self), FU_DEVICE_REMOVE_DELAY_RE_ENUMERATE);
}

static void
fu_thelio_io_device_class_init (FuThelioIoDeviceClass *klass)
{
	FuDeviceClass *klass_device = FU_DEVICE_CLASS (klass);
	klass_device->probe = fu_thelio_io_device_probe;
	klass_device->detach = fu_thelio_io_device_detach;
}

FuThelioIoDevice *
fu_thelio_io_device_new (FuUsbDevice *device)
{
	FuThelioIoDevice *self = g_object_new (FU_TYPE_THELIO_IO_DEVICE, NULL);
	fu_device_incorporate (FU_DEVICE (self), FU_DEVICE (device));
	return self;
}
