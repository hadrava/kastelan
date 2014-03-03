/*
 * Compute odometry from interrupts on gpio lines
 *
 * To use, just declare in your board resources:
 * static struct resource foo_resources[] = {
 *     .start = 0,
 *     .end = 5,
 *     .flags = IORESOURCE_IRQ,
 * };
 * static struct platform_device foo_dev = {
 *     .name = "srv1-encoder",
 *     .num_resources = 1,
 *     .resource = &foo_resources
 * };
 * This will setup GPIO0 - GPIO5 (inclusive) for use by this driver.
 *
 * Copyright 2012 Jan Hadrava
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/interrupt.h>

#include <asm/atomic.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>

#define stamp(fmt, args...) pr_debug("%s:%i: " fmt "\n", __func__, __LINE__, ## args)
#define pr_devinit(fmt, args...) ({ static const __devinitconst char __fmt[] = fmt; printk(__fmt, ## args); })
#define pr_init(fmt, args...) ({ static const __initconst char __fmt[] = fmt; printk(__fmt, ## args); })

#define DRIVER_NAME "srv1-encoder"
#define PFX DRIVER_NAME ": "

#define ENCODER_LEFT_M 0
#define ENCODER_RIGHT_M 1

#define ENCODER_LEFT_PIN_A       GPIO_PH8
#define ENCODER_LEFT_PIN_B       GPIO_PH9
#define ENCODER_RIGHT_PIN_A     GPIO_PH10
#define ENCODER_RIGHT_PIN_B     GPIO_PH11

#define ENCODER_LEFT_PIN_A_IRQ    IRQ_PH8
#define ENCODER_LEFT_PIN_B_IRQ    IRQ_PH9
#define ENCODER_RIGHT_PIN_A_IRQ  IRQ_PH10
#define ENCODER_RIGHT_PIN_B_IRQ  IRQ_PH11

static volatile int  encoder_left        = 0;
static volatile int  encoder_right       = 0;
static volatile int  encoder_error_left  = 0;
static volatile int  encoder_error_right = 0;
static volatile char encoder_raw_left    = 0;
static volatile char encoder_raw_right   = 0;

struct encoder_data {
  atomic_t open_count;
};
struct group_data {
  dev_t dev_node;
  struct cdev cdev;
  struct resource *encoder_range;
  struct encoder_data *encoders;
};

/**
*	srv1_encoder_read - sample the value of the specified encoder
*/
static ssize_t srv1_encoder_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
  unsigned int encoder = iminor(file->f_path.dentry->d_inode);

  if (encoder == ENCODER_LEFT_M) {
    put_user(encoder_left, buf);
    put_user(encoder_left >> 8, buf + 1);
    put_user(encoder_left >> 16, buf + 2);
    put_user(encoder_left >> 24, buf + 3);
  } else if (encoder == ENCODER_RIGHT_M) {
    put_user(encoder_right, buf);
    put_user(encoder_right >> 8, buf + 1);
    put_user(encoder_right >> 16, buf + 2);
    put_user(encoder_right >> 24, buf + 3);
  } else
    return -ENODEV;

  return sizeof(int);
}

//irq
static irqreturn_t srv1_encoder_handle_interrupt_left(int irq, void *data)
{
  encoder_raw_left |= (gpio_get_value(ENCODER_LEFT_PIN_A) << 3) | (gpio_get_value(ENCODER_LEFT_PIN_B) << 2);
  switch (encoder_raw_left) {
    case 0x2:
    case 0xB:
    case 0xD:
    case 0x4:
      encoder_left--;
      break;
    case 0x1:
    case 0x8:
    case 0xE:
    case 0x7:
      encoder_left++;
      break;
    case 0x3:
    case 0x6:
    case 0xC:
    case 0x9:
      encoder_error_left++;
      break;
    default:
      break;
  }
  encoder_raw_left = encoder_raw_left >> 2;
  return IRQ_HANDLED;
}

static irqreturn_t srv1_encoder_handle_interrupt_right(int irq, void *data)
{
  encoder_raw_right |= (gpio_get_value(ENCODER_RIGHT_PIN_A) << 3) | (gpio_get_value(ENCODER_RIGHT_PIN_B) << 2);
  switch (encoder_raw_right) {
  case 0x2:
  case 0xB:
  case 0xD:
  case 0x4:
    encoder_right++;
    break;
  case 0x1:
  case 0x8:
  case 0xE:
  case 0x7:
    encoder_right--;
    break;
  case 0x3:
  case 0x6:
  case 0xC:
  case 0x9:
    encoder_error_right++;
    break;
  }
  encoder_raw_right = encoder_raw_right >> 2;
  return IRQ_HANDLED;
}

/**
 *	srv1_encoder_write - modify the state of the specified encoder
 *
 *	We allow people to control the direction and value of the specified encoder.
 *	You can only change these once per write though.  We process the user buf
 *	and only do the last thing specified.  We also ignore newlines.  This way
 *	you can quickly test by doing from the shell:
 *		$ echo 'O1' > /dev/encoder8
 *	This will set encoder8 to ouput mode and set the value to 1.
 */
static ssize_t srv1_encoder_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
	unsigned int encoder = iminor(file->f_path.dentry->d_inode);
	ssize_t ret;
	char dir = '?';

	ret = 0;
	while (ret < count) {
		char byte;
		int user_ret = get_user(byte, buf + ret++);
		if (user_ret)
			return user_ret;

		switch (byte) {
		case '\r':
		case '\n': continue;
		case 'R': dir = byte; break;
		case 'E': dir = byte; break;
		default:  return -EINVAL;
		}
		stamp("processed byte '%c'", byte);
	}

	switch (dir) {
	case 'R':
	  if (encoder == ENCODER_LEFT_M)
            encoder_left = 0;
	  else if (encoder == ENCODER_RIGHT_M)
	    encoder_right = 0;
	  break;
	case 'E':
	  if (encoder == ENCODER_LEFT_M)
            encoder_left = encoder_error_left;
	  else if (encoder == ENCODER_RIGHT_M)
	    encoder_right = encoder_error_right;
	  break;
	}
	return ret;
}


/**
 *	srv1_encoder_open - claim the specified encoder
 *
 *	Grab the specified encoder if it's available and keep track of how many times
 *	we've been opened (see close() below).  We allow multiple people to open
 *	at the same time as there's no real limitation in the hardware for reading
 *	from different processes.  Plus this way you can have one app do the write
 *	and management while quickly monitoring from another by doing:
 *		$ cat /dev/encoder8
 */
static int srv1_encoder_open(struct inode *ino, struct file *file) {
  struct group_data *group_data = container_of(ino->i_cdev, struct group_data, cdev);
  unsigned int encoder = iminor(ino);
  struct encoder_data *encoder_data = &group_data->encoders[encoder - group_data->encoder_range->start];

  if (encoder != ENCODER_LEFT_M && encoder != ENCODER_RIGHT_M)
    return -ENODEV;

  atomic_inc(&encoder_data->open_count);

  return 0;
}

/**
 *	srv1_encoder_close - release the specified encoder
 *
 *	Do not actually free the specified encoder until the last person has closed.
 *	We claim/release here rather than during probe() so that people can swap
 *	between drivers on the fly during runtime without having to load/unload
 *	kernel modules.
 */
static int srv1_encoder_release(struct inode *ino, struct file *file) {
  struct group_data *group_data = container_of(ino->i_cdev, struct group_data, cdev);
  unsigned int encoder = iminor(ino);
  struct encoder_data *encoder_data = &group_data->encoders[encoder - group_data->encoder_range->start];



  /* do not free until last consumer has closed */
  if (!atomic_dec_and_test(&encoder_data->open_count))
    stamp("encoder still in use -- not freeing");

  return 0;
}

static struct class *srv1_encoder_class;

static struct file_operations srv1_encoder_fops = {
  .owner    = THIS_MODULE,
  .read     = srv1_encoder_read,
  .write    = srv1_encoder_write,
  .open     = srv1_encoder_open,
  .release  = srv1_encoder_release,
};

/**
 *	srv1_encoder_probe - setup the range of encoders
 *
 *	Create a character device for the range of encoders and have the minor be
 *	used to specify the encoder.
 */
static int __devinit srv1_encoder_probe(struct platform_device *pdev) {
  int ret;
  struct group_data *group_data;
  struct resource *encoder_range = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
  int encoder, encoder_max = encoder_range->end - encoder_range->start + 1;



  group_data = kzalloc(sizeof(*group_data) + sizeof(struct encoder_data) * encoder_max, GFP_KERNEL);
  if (!group_data)
    return -ENOMEM;
  group_data->encoder_range = encoder_range;
  group_data->encoders = (void *)group_data + sizeof(*group_data);
  platform_set_drvdata(pdev, group_data);

  ret = alloc_chrdev_region(&group_data->dev_node, encoder_range->start, encoder_max, "encoder");
  if (ret) {
    pr_devinit(KERN_ERR PFX "unable to get a char device\n");
    kfree(group_data);
    return ret;
  }

  cdev_init(&group_data->cdev, &srv1_encoder_fops);
  group_data->cdev.owner = THIS_MODULE;

  ret = cdev_add(&group_data->cdev, group_data->dev_node, encoder_max);
  if (ret) {
    pr_devinit(KERN_ERR PFX "unable to register char device\n");
    kfree(group_data);
    unregister_chrdev_region(group_data->dev_node, encoder_max);
    return ret;
  }

  for (encoder = encoder_range->start; encoder <= encoder_range->end; ++encoder)
    device_create(srv1_encoder_class, &pdev->dev, group_data->dev_node + encoder,
	NULL, "enc%i", encoder);

  device_init_wakeup(&pdev->dev, 1);

  pr_devinit(KERN_INFO PFX "now handling %i encoders: %i - %i\n",
      encoder_max, encoder_range->start, encoder_range->end);

  return 0;
}

/**
 *	srv1_encoder_remove - break down the range of encoders
 *
 *	Release the character device and related pieces for this range of encoders.
 */
static int __devexit srv1_encoder_remove(struct platform_device *pdev) {
  struct group_data *group_data = platform_get_drvdata(pdev);
  struct resource *encoder_range = group_data->encoder_range;
  int encoder;

  gpio_free(ENCODER_LEFT_PIN_A);
  gpio_free(ENCODER_LEFT_PIN_B);
  gpio_free(ENCODER_RIGHT_PIN_A);
  gpio_free(ENCODER_RIGHT_PIN_B);

  for (encoder = encoder_range->start; encoder <= encoder_range->end; ++encoder)
    device_destroy(srv1_encoder_class, group_data->dev_node + encoder);

  cdev_del(&group_data->cdev);
  unregister_chrdev_region(group_data->dev_node, 1);

  kfree(group_data);

  return 0;
}

struct platform_driver srv1_encoder_device_driver = {
  .probe   = srv1_encoder_probe,
  .remove  = __devexit_p(srv1_encoder_remove),
  .driver  = {
    .name = DRIVER_NAME,
  }
};

/**
 *	srv1_encoder_init - setup our odometry device driver
 *
 *	Create one odometry class for the entire driver
 */
static int __init srv1_encoder_init(void) {
  int ret;

  srv1_encoder_class = class_create(THIS_MODULE, DRIVER_NAME);
  if (IS_ERR(srv1_encoder_class)) {
    pr_init(KERN_ERR PFX "Unable to create encoder class\n");
    return PTR_ERR(srv1_encoder_class);
  }

  gpio_request(ENCODER_LEFT_PIN_A, DRIVER_NAME);
  gpio_request(ENCODER_LEFT_PIN_B, DRIVER_NAME);
  gpio_direction_input(ENCODER_LEFT_PIN_A);
  gpio_direction_input(ENCODER_LEFT_PIN_B);
  encoder_raw_left = (gpio_get_value(ENCODER_LEFT_PIN_A) << 1) | gpio_get_value(ENCODER_LEFT_PIN_B);
  ret = request_irq(ENCODER_LEFT_PIN_A_IRQ,  srv1_encoder_handle_interrupt_left,  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "srv1-encoder-left", NULL);
  if (ret)
    printk("srv1-encoder: failed to request left A irq (%i)\n", ret);
  ret = request_irq(ENCODER_LEFT_PIN_B_IRQ,  srv1_encoder_handle_interrupt_left,  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "srv1-encoder-left", NULL);
  if (ret)
    printk("srv1-encoder: failed to request left B irq (%i)\n", ret);

  gpio_request(ENCODER_RIGHT_PIN_A, DRIVER_NAME);
  gpio_request(ENCODER_RIGHT_PIN_B, DRIVER_NAME);
  gpio_direction_input(ENCODER_RIGHT_PIN_A);
  gpio_direction_input(ENCODER_RIGHT_PIN_B);
  encoder_raw_right = (gpio_get_value(ENCODER_RIGHT_PIN_A) << 1) | gpio_get_value(ENCODER_RIGHT_PIN_B);
  ret = request_irq(ENCODER_RIGHT_PIN_A_IRQ, srv1_encoder_handle_interrupt_right, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "srv1-encoder-right", NULL);
  if (ret)
    printk("srv1-encoder: failed to request right A irq (%i)\n", ret);
  ret = request_irq(ENCODER_RIGHT_PIN_B_IRQ, srv1_encoder_handle_interrupt_right, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "srv1-encoder-right", NULL);
  if (ret)
    printk("srv1-encoder: failed to request right B irq (%i)\n", ret);

  return platform_driver_register(&srv1_encoder_device_driver);
}
module_init(srv1_encoder_init);

static void __exit srv1_encoder_exit(void) {
  class_destroy(srv1_encoder_class);

  platform_driver_unregister(&srv1_encoder_device_driver);
}
module_exit(srv1_encoder_exit);

MODULE_AUTHOR("Jan Hadrava <jhadrava@gmail.com>");
MODULE_DESCRIPTION("Odometry interface for robot");
MODULE_LICENSE("GPL");
