/*
 * File:         drivers/media/video/blackfin/ov9655.c
 * Based on:
 * Author:       Martin Strubel <hackfin@section5.ch>
 *
 * Created:
 * Description:  Command driver for Omnivision OV9655 sensor
 *
 *
 * Modified:
 *               10/2007 Omnivision 9655 basic image driver
 *
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define DEBUG

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/wait.h>
#include <linux/videodev.h>
#include <media/v4l2-dev.h>

#include "ov9655.h"

/*static char myconfig[] = {
0x12, 0x80,
0x3d, 0x03,
0x17, 0x22,
0x18, 0xa4,
0x19, 0x07,
0x1a, 0xf0,
0x32, 0x00,
0x29, 0x50,
0x2c, 0x78,
0x2a, 0x00,
0x11, 0x03, // 00/01/03/07 for 60/30/15/7.5fps  - set to 15fps for QVGA
0x42, 0x7f,
0x4d, 0x09,
0x63, 0xe0,
0x64, 0xff,
0x65, 0x2f,
0x0c, 0x00, // flip Y with UV
0x66, 0x00, // flip Y with UV
0x67, 0x48,
0x13, 0xf0,
0x0d, 0x61, // 51/61/71 for different AEC/AGC window
0x0f, 0xc5,
0x14, 0x11,
0x22, 0x3f, // ff/7f/3f/1f for 60/30/15/7.5fps
0x23, 0x07, // 01/03/07/0f for 60/30/15/7.5fps
0x24, 0x40,
0x25, 0x30,
0x26, 0xa1,
0x2b, 0x00, // 00/9e for 60/50Hz
0x6b, 0xaa,
0x13, 0xff,
0x90, 0x05,
0x91, 0x01,
0x92, 0x03,
0x93, 0x00,
0x94, 0xb0,
0x95, 0x9d,
0x96, 0x13,
0x97, 0x16,
0x98, 0x7b,
0x99, 0x91,
0x9a, 0x1e,
0x9b, 0x08,
0x9c, 0x20,
0x9e, 0x81,
0xa6, 0x04,
0x7e, 0x0c,
0x7f, 0x16,
0x80, 0x2a,
0x81, 0x4e,
0x82, 0x61,
0x83, 0x6f,
0x84, 0x7b,
0x85, 0x86,
0x86, 0x8e,
0x87, 0x97,
0x88, 0xa4,
0x89, 0xaf,
0x8a, 0xc5,
0x8b, 0xd7,
0x8c, 0xe8,
0x8d, 0x20,
};*/

static char myconfig_sxga[] = {
0x00, 0x00,
0x01, 0x80,
0x02, 0x80,
0x03, 0x1b,
0x04, 0x03,
0x0b, 0x57,
0x0e, 0x61,
0x0f, 0x40,
0x11, 0x01,
0x12, 0x02,
0x13, 0xc7,  // was e7 - turned banding filter off
0x14, 0x3a,
0x16, 0x24,
0x17, 0x1d,
0x18, 0xbd,
0x19, 0x01,
0x1a, 0x81,
0x1e, 0x04,
0x24, 0x3c,
0x25, 0x36,
0x26, 0x72,
0x27, 0x08,
0x28, 0x08,
0x29, 0x15,
0x2a, 0x00,
0x2b, 0x00,
0x2c, 0x08,
0x32, 0xff,
0x33, 0x00,
0x34, 0x3d,
0x35, 0x00,
0x36, 0xf8,
0x38, 0x72,
0x39, 0x57,
0x3a, 0x8c,  // UYVY capture
0x3b, 0x04,
0x3d, 0x99,
0x3e, 0x0c,
0x3f, 0xc1,
0x40, 0xc0,
0x41, 0x00,
0x42, 0xc0,
0x43, 0x0a,
0x44, 0xf0,
0x45, 0x46,
0x46, 0x62,
0x47, 0x2a,
0x48, 0x3c,
0x4a, 0xfc,
0x4b, 0xfc,
0x4c, 0x7f,
0x4d, 0x7f,
0x4e, 0x7f,
0x4f, 0x98,
0x50, 0x98,
0x51, 0x00,
0x52, 0x28,
0x53, 0x88,
0x54, 0xb0,
0x58, 0x1a,
0x58, 0x1a,
0x59, 0x85,
0x5a, 0xa9,
0x5b, 0x64,
0x5c, 0x84,
0x5d, 0x53,
0x5e, 0x0e,
0x5f, 0xf0,
0x60, 0xf0,
0x61, 0xf0,
0x62, 0x00,
0x63, 0x00,
0x64, 0x02,
0x65, 0x16,
0x66, 0x01,
0x69, 0x02,
0x6b, 0x5a,
0x6c, 0x04,
0x6d, 0x55,
0x6e, 0x00,
0x6f, 0x9d,
0x70, 0x21,
0x71, 0x78,
0x72, 0x00,
0x73, 0x01,
0x74, 0x3a,
0x75, 0x35,
0x76, 0x01,
0x77, 0x02,
0x7a, 0x12,
0x7b, 0x8,
0x7c, 0x15,
0x7d, 0x24,
0x7e, 0x45,
0x7f, 0x55,
0x80, 0x6a,
0x81, 0x78,
0x82, 0x87,
0x83, 0x96,
0x84, 0xa3,
0x85, 0xb4,
0x86, 0xc3,
0x87, 0xd6,
0x88, 0xe6,
0x89, 0xf2,
0x8a, 0x03,
0x8c, 0x0d,
0x90, 0x7d,
0x91, 0x7b,
0x9d, 0x03,
0x9e, 0x04,
0x9f, 0x7a,
0xa0, 0x79,
0xa1, 0x10,   //  changes exposure time - default was 0x40
0xa4, 0x50,
0xa5, 0x68,
0xa6, 0x4a,
0xa8, 0xc1,
0xa9, 0xef,
0xaa, 0x92,
0xab, 0x04,
0xac, 0x80,
0xad, 0x80,
0xae, 0x80,
0xaf, 0x80,
0xb2, 0xf2,
0xb3, 0x20,
0xb4, 0x20,
0xb5, 0x00,
0xb6, 0xaf,
0xbb, 0xae,
0xbc, 0x7f,
0xbd, 0x7f,
0xbe, 0x7f,
0xbf, 0x7f,
0xc0, 0xe2,
0xc1, 0xc0,
0xc2, 0x01,
0xc3, 0x4e,
0xc6, 0x05,
0xc7, 0x80,
0xc9, 0xe0,
0xca, 0xe8,
0xcb, 0xf0,
0xcc, 0xd8,
0xcd, 0x93,
};

static unsigned char myconfig_vga[] = {
                0x00, 0x00,
                0x01, 0x80,
                0x02, 0x80,
                0x03, 0x12,
                0x04, 0x03,
                0x0b, 0x57,
                0x0e, 0x61,
                0x0f, 0x40,
                0x11, 0x01,
                0x12, 0x62,
                0x13, 0xc7,
                0x14, 0x3a,
                0x16, 0x24,
                0x17, 0x16,
                0x18, 0x02,
                0x19, 0x01,
                0x1a, 0x3d,
                0x1e, 0x04,
                0x24, 0x3c,
                0x25, 0x36,
                0x26, 0x72,
                0x27, 0x08,
                0x28, 0x08,
                0x29, 0x15,
                0x2a, 0x00,
                0x2b, 0x00,
                0x2c, 0x08,
                0x32, 0xff,
                0x33, 0x00,
                0x34, 0x3F,
                0x35, 0x00,
                0x36, 0xfa,
                0x38, 0x72,
                0x39, 0x57,
                0x3a, 0x8c,
                0x3b, 0x04,
                0x3d, 0x99,
                0x3e, 0x0c,
                0x3f, 0xc1,
                0x40, 0xc0,
                0x41, 0x00,
                0x42, 0xc0,
                0x43, 0x0a,
                0x44, 0xf0,
                0x45, 0x46,
                0x46, 0x62,
                0x47, 0x2a,
                0x48, 0x3c,
                0x4a, 0xfc,
                0x4b, 0xfc,
                0x4c, 0x7f,
                0x4d, 0x7f,
                0x4e, 0x7f,
                0x4f, 0x98,
                0x50, 0x98,
                0x51, 0x00,
                0x52, 0x28,
                0x53, 0x70,
                0x54, 0x98,
                0x58, 0x1a,
                0x59, 0x85,
                0x5a, 0xa9,
                0x5b, 0x64,
                0x5c, 0x84,
                0x5d, 0x53,
                0x5e, 0x0e,
                0x5f, 0xf0,
                0x60, 0xf0,
                0x61, 0xf0,
                0x62, 0x00,
                0x63, 0x00,
                0x64, 0x02,
                0x65, 0x20,
                0x66, 0x00,
                0x69, 0x0a,
                0x6b, 0x5a,
                0x6c, 0x04,
                0x6d, 0x55,
                0x6e, 0x00,
                0x6f, 0x9d,
                0x70, 0x21,
                0x71, 0x78,
                0x72, 0x00,
                0x73, 0x00,
                0x74, 0x3a,
                0x75, 0x35,
                0x76, 0x01,
                0x77, 0x02,
                0x7A, 0x12,
                0x7B, 0x08,
                0x7C, 0x16,
                0x7D, 0x30,
                0x7E, 0x5e,
                0x7F, 0x72,
                0x80, 0x82,
                0x81, 0x8e,
                0x82, 0x9a,
                0x83, 0xa4,
                0x84, 0xac,
                0x85, 0xb8,
                0x86, 0xc3,
                0x87, 0xd6,
                0x88, 0xe6,
                0x89, 0xf2,
                0x8a, 0x24,
                0x8c, 0x8d,
                0x90, 0x7d,
                0x91, 0x7b,
                0x9d, 0x02,
                0x9e, 0x02,
                0x9f, 0x7a,
                0xa0, 0x79,
                0xa1, 0x40,
                0xa4, 0x50,
                0xa5, 0x68,
                0xa6, 0x4a,
                0xa8, 0xc1,
                0xa9, 0xef,
                0xaa, 0x92,
                0xab, 0x04,
                0xac, 0x80,
                0xad, 0x80,
                0xae, 0x80,
                0xaf, 0x80,
                0xb2, 0xf2,
                0xb3, 0x20,
                0xb4, 0x20,
                0xb5, 0x00,
                0xb6, 0xaf,
                0xbb, 0xae,
                0xbc, 0x7f,
                0xbd, 0x7f,
                0xbe, 0x7f,
                0xbf, 0x7f,
                0xc0, 0xaa,
                0xc1, 0xc0,
                0xc2, 0x01,
                0xc3, 0x4e,
                0xc6, 0x05,
                0xc7, 0x80,
                0xc9, 0xe0,
                0xca, 0xe8,
                0xcb, 0xf0,
                0xcc, 0xd8,
                0xcd, 0x93,
                0xcd, 0x93,
};


static DEFINE_MUTEX(ov9655_sysfs_lock);

static int SCCB_Write(struct i2c_client *client,
				 u8 offset, u8 data)
{
	u8 buf[2];

	BUG_ON(client == NULL);

	buf[0] = offset;
	buf[1] = data;

	i2c_master_send(client, buf, 2);

	return 0;
}

static int SCCB_Read(struct i2c_client *client,
				u8 offset, u8 *data)
{
	BUG_ON(client == NULL);

	i2c_smbus_write_byte(client, offset);
	i2c_master_recv(client, data, 1);
}

static int my_probe(struct i2c_client *client)
{

	u8 buf = 0;
	/* Only read MSB product ID register, as LSB is revision dependent */
	SCCB_Read(client, 0x0A, &buf);

	pr_debug("OV9655: PID register contained 0x%02X\n",buf);

	if (buf == OV9655_PID_MSB)
		return 0;

	return -ENODEV;
}

static int my_set_pixfmt(struct i2c_client *client, u32 arg)
{
	return 0;
}

static int my_set_framerate(struct i2c_client *client, u32 arg)
{
	return -EPERM;
}

static int my_set_window(struct i2c_client *client, u32 res)
{
	return 0;
}

static int my_set_resolution(struct i2c_client *client, u32 res)
{
	const char *b, *end;

	pr_debug("Setting resolution to %dx%d\n",X_RES(res),Y_RES(res));

	switch (res) {
	case RES_SXGA:
		pr_debug("SXGA resolution-set routine called\n");

		b = myconfig_sxga;
		end = &b[sizeof(myconfig_sxga)];

		while (b < end) {
			SCCB_Write(client, b[0], b[1]);
			b += 2;
		}

		break;
	case RES_VGA:
		pr_debug("VGA resolution-set routine called\n");

		b = myconfig_vga;
		end = &b[sizeof(myconfig_vga)];

		while (b < end) {
			SCCB_Write(client, b[0], b[1]);
			b += 2;
		}

		break;

	default:
		my_set_window(client, res);

	}

	msleep(300);

	return 0;
}

static int my_init(struct i2c_client *client, u32 arg)
{
	const char *b, *end;

	if (my_probe(client))
		return -ENODEV;

	SCCB_Write(client, 0x12, 0x80); /* reset */

	msleep(100);

	pr_debug("Init settings downloaded: %d\n",6767);

	b = myconfig_sxga;
	end = &b[sizeof(myconfig_sxga)];

	while (b < end) {
		SCCB_Write(client, b[0], b[1]);
		b += 2;
	}

	return 0;

}

static int my_exit(struct i2c_client *client, u32 arg)
{
	return 0;
}

int cam_control(struct i2c_client *client, u32 cmd, u32 arg)
{
	switch (cmd) {
	case CAM_CMD_INIT:
		return my_init(client, arg);
	case CAM_CMD_SET_RESOLUTION:
		return my_set_resolution(client, arg);
	case CAM_CMD_SET_FRAMERATE:
		return my_set_framerate(client, arg);
	case CAM_CMD_SET_PIXFMT:
		return my_set_pixfmt(client, arg);
	case CAM_CMD_EXIT:
		return my_exit(client, arg);
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

/****************************************************************************
 *  sysfs
 ***************************************************************************/

static u8 sysfs_strtou8(const char *buff, size_t len, ssize_t *count)
{
	char str[5];
	char *endp;
	unsigned long val;

	if (len < 4) {
		strncpy(str, buff, len);
		str[len + 1] = '\0';
	} else {
		strncpy(str, buff, 4);
		str[4] = '\0';
	}

	val = simple_strtoul(str, &endp, 0);

	*count = 0;
	if (val <= 0xff)
		*count = (ssize_t) (endp - str);
	if ((*count) && (len == *count + 1) && (buff[*count] == '\n'))
		*count += 1;

	return (u8) val;
}

static ssize_t sysfs_show_val(struct device *cd, struct device_attribute *attr, char *buf, int cmd)
{
	struct bcap_device_t *cam;
	ssize_t count;
	u8 val[1];

	if (mutex_lock_interruptible(&ov9655_sysfs_lock))
		return -ERESTARTSYS;

	cam = video_get_drvdata(to_video_device(cd));
	if (!cam) {
		mutex_unlock(&ov9655_sysfs_lock);
		return -ENODEV;
	}

	if (cam_control(cam->client, cmd, (u32) val) < 0) {
		mutex_unlock(&ov9655_sysfs_lock);
		return -EIO;
	}

	count = sprintf(buf, "%d\n", val[0]);

	mutex_unlock(&ov9655_sysfs_lock);

	return count;
}

static ssize_t
sysfs_store_val(struct device *cd, struct device_attribute *attr, const char *buf, size_t len,
		      int cmd)
{
	struct bcap_device_t *cam;
	u8 value;
	ssize_t count;
	int err;

	if (mutex_lock_interruptible(&ov9655_sysfs_lock))
		return -ERESTARTSYS;

	cam = video_get_drvdata(to_video_device(cd));

	if (!cam) {
		mutex_unlock(&ov9655_sysfs_lock);
		return -ENODEV;
	}

	value = sysfs_strtou8(buf, len, &count);

	if (!count) {
		mutex_unlock(&ov9655_sysfs_lock);
		return -EINVAL;
	}

	err = cam_control(cam->client, cmd, value);

	if (err) {
		mutex_unlock(&ov9655_sysfs_lock);
		return -EIO;
	}

	mutex_unlock(&ov9655_sysfs_lock);

	return count;
}

static ssize_t sysfs_fps_show(struct device *cd, struct device_attribute *attr, char *buf)
{

	return sysfs_show_val(cd, attr, buf, CAM_CMD_GET_FRAMERATE);
}

static ssize_t
sysfs_fps_store(struct device *cd, struct device_attribute *attr, const char *buf, size_t len)
{
	return sysfs_store_val(cd, attr, buf, len, CAM_CMD_SET_FRAMERATE);
}

static DEVICE_ATTR(fps, S_IRUGO | S_IWUSR,
			 sysfs_fps_show, sysfs_fps_store);

static ssize_t sysfs_flicker_show(struct device *cd, struct device_attribute *attr, char *buf)
{
	return sysfs_show_val(cd, attr, buf, CAM_CMD_GET_FLICKER_FREQ);
}

static ssize_t
sysfs_flicker_store(struct device *cd, struct device_attribute *attr, const char *buf, size_t len)
{
	return sysfs_store_val(cd, attr, buf, len, CAM_CMD_SET_FLICKER_FREQ);
}

static DEVICE_ATTR(flicker, S_IRUGO | S_IWUSR,
			 sysfs_flicker_show, sysfs_flicker_store);

static ssize_t sysfs_h_mirror_show(struct device *cd, struct device_attribute *attr, char *buf)
{
	return sysfs_show_val(cd, attr, buf, CAM_CMD_GET_HOR_MIRROR);
}

static ssize_t
sysfs_h_mirror_store(struct device *cd, struct device_attribute *attr, const char *buf, size_t len)
{
	return sysfs_store_val(cd, attr, buf, len, CAM_CMD_SET_HOR_MIRROR);
}

static DEVICE_ATTR(h_mirror, S_IRUGO | S_IWUSR,
			 sysfs_h_mirror_show, sysfs_h_mirror_store);

static ssize_t sysfs_v_mirror_show(struct device *cd, struct device_attribute *attr, char *buf)
{
	return sysfs_show_val(cd, attr, buf, CAM_CMD_GET_VERT_MIRROR);
}

static ssize_t
sysfs_v_mirror_store(struct device *cd, struct device_attribute *attr, const char *buf, size_t len)
{
	return sysfs_store_val(cd, attr, buf, len, CAM_CMD_SET_VERT_MIRROR);
}

static DEVICE_ATTR(v_mirror, S_IRUGO | S_IWUSR,
			 sysfs_v_mirror_show, sysfs_v_mirror_store);

static int ov9655_create_sysfs(struct video_device *v4ldev, int action)
{

	int rc;

	rc = device_create_file(&v4ldev->dev, &dev_attr_fps);
	if (rc)
		goto err;
	rc = device_create_file(&v4ldev->dev, &dev_attr_flicker);
	if (rc)
		goto err_flicker;
	rc = device_create_file(&v4ldev->dev, &dev_attr_v_mirror);
	if (rc)
		goto err_v_mirror;
	rc = device_create_file(&v4ldev->dev, &dev_attr_h_mirror);
	if (rc)
		goto err_h_mirror;

	return 0;

err_h_mirror:
	device_remove_file(&v4ldev->dev, &dev_attr_v_mirror);
err_v_mirror:
	device_remove_file(&v4ldev->dev, &dev_attr_flicker);
err_flicker:
	device_remove_file(&v4ldev->dev, &dev_attr_fps);
err:
	return rc;
}

static struct bcap_camera_ops ov9655_ops = {
	cam_control,
	ov9655_create_sysfs,
	NULL,
};

struct bcap_camera_ops *get_camops(void)
{
	return (&ov9655_ops);

}
EXPORT_SYMBOL(get_camops);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Hennerich <hennerich@blackfin.uclinux.org>");
