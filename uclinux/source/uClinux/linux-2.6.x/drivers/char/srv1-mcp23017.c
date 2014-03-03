/*
 * Keyboard driver
 *
 * To use, just declare in your board resources:
 * static struct resource foo_resources[] = {
 *     .start = 0,
 *     .end = 5,
 *     .flags = IORESOURCE_IRQ,
 * };
 * static struct platform_device foo_dev = {
 *     .name = "srv1-mcp23017",
 *     .num_resources = 1,
 *     .resource = &foo_resources
 * };
 * This will setup GPIO_PH15 for interrupt
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
#include <linux/i2c.h>

#include <asm/atomic.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>

#define stamp(fmt, args...) pr_debug("%s:%i: " fmt "\n", __func__, __LINE__, ## args)
#define pr_devinit(fmt, args...) ({ static const __devinitconst char __fmt[] = fmt; printk(__fmt, ## args); })
#define pr_init(fmt, args...) ({ static const __initconst char __fmt[] = fmt; printk(__fmt, ## args); })

#define DRIVER_NAME "srv1-mcp23017"
#define PFX DRIVER_NAME ": "

#define MCP_MINOR                       0
#define MCP_INT_PIN             GPIO_PH15
#define MCP_INT_PIN_IRQ          IRQ_PH15
#define MCP_GPIOB                    0x13
#define MCP_IODIRB                   0x01
#define MCP_INTCONB                  0x09
#define MCP_IOCON                    0x0A
#define MCP_DEFVALB                  0x07
#define MCP_GPINTENB                 0x05
#define MCP_GPPUB                    0x0D
#define MCP_ADDRESS_GPIO             0x27

static int srv1_mcp23017_attach_adapter(struct i2c_adapter *adapter);
static int srv1_mcp23017_detach_client(struct i2c_client *client);
static void srv1_mcp23017_get_keycode(void);

static struct i2c_client *toto_je_fuj_client;
static wait_queue_head_t toto_je_fuj_wq;

static u16 normal_i2c[] = {
  MCP_ADDRESS_GPIO,
  MCP_ADDRESS_GPIO,
  I2C_CLIENT_END
};
I2C_CLIENT_INSMOD;

static struct i2c_driver srv1_mcp23017_driver = {
  .driver = {
    .name = "srv1_mcp23017",
  },
  .attach_adapter = srv1_mcp23017_attach_adapter,
  .detach_client  = srv1_mcp23017_detach_client,
};

static volatile unsigned char mcp23017_buffer[8];
static volatile int mcp23017_bufflen = 0;
static volatile int mcp23017_should_wake = 0;

struct mcp23017_data {
  struct i2c_client	client;
  atomic_t open_count;
};
struct group_data {
  dev_t dev_node;
  struct cdev cdev;
  struct resource *mcp23017_range;
  struct mcp23017_data *mcp23017;
};

/**
*	srv1_mcp23017_read - read keyboard buffer
*/
static ssize_t srv1_mcp23017_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
  unsigned int mcp23017_minor = iminor(file->f_path.dentry->d_inode);
  ssize_t ret;

  if (wait_event_interruptible(toto_je_fuj_wq, mcp23017_should_wake == 1))
    return -ERESTARTSYS;

  srv1_mcp23017_get_keycode();
  mcp23017_should_wake = 0;

  ret = 0;
  if (mcp23017_minor == MCP_MINOR) {
    while (mcp23017_bufflen) {
      // TODO atomic
      mcp23017_bufflen --;
      put_user(mcp23017_buffer[mcp23017_bufflen], buf + ret);
      ret++;
    }
  } else
    return -ENODEV;
  return ret;
}

static int srv1_mcp23017_detect_client(struct i2c_adapter *adapter, int address, int kind) {
  int ret;
  struct i2c_client *new_client;
  struct mcp23017_data *data;
  printk("srv1-mcp23017: srv1_mcp23017_detect_client: init\n");

  data = kzalloc(sizeof(struct mcp23017_data), GFP_KERNEL);
  if (data == NULL)
    return -ENOMEM;

  new_client = &data->client;

  if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
    return 0;


  printk("srv1-mcp23017: srv1_mcp23017_detect_client: bp1\n");
  i2c_set_clientdata(new_client, data);
  new_client->addr = address;
  new_client->adapter = adapter;
  new_client->driver = &srv1_mcp23017_driver;
  printk("srv1-mcp23017: srv1_mcp23017_detect_client: bp2\n");
  strcpy(new_client->name, new_client->driver->driver.name);

  printk("srv1-mcp23017: srv1_mcp23017_detect_client: bp3\n");
  ret = i2c_attach_client(new_client);
  toto_je_fuj_client = new_client;
  printk("srv1-mcp23017: srv1_mcp23017_detect_client: bp4\n");
  if (ret) {
    kfree(data);
    return ret;
  }

  return 0;
}

static int srv1_mcp23017_attach_adapter(struct i2c_adapter *adapter) {
  printk("srv1-mcp23017: srv1_mcp23017_attach_adapter: init\n");
  return i2c_probe(adapter, &addr_data, &srv1_mcp23017_detect_client);
}

static int srv1_mcp23017_detach_client(struct i2c_client *client) {
  struct mcp23017_data *data;
  int ret;

  ret = i2c_detach_client(client);
  if (ret)
    return ret;

  data = i2c_get_clientdata(client);
  kfree(data);
  return 0;
}

void set_register(unsigned char reg, unsigned char set) {
  unsigned char buf[4];
  buf[0] = reg;
  buf[1] = set;

  if (i2c_master_send(toto_je_fuj_client, buf, 2) != 2) {//konflikt todo
    pr_devinit(KERN_ERR PFX "chyba write do mcp\n");
    return;
  }
}

unsigned char get_register(unsigned char reg) {
  unsigned char val;
  if (i2c_master_send(toto_je_fuj_client, &reg, 1) != 1) {//konflikt todo
    pr_devinit(KERN_ERR PFX "chyba write do mcp\n");
    return 0;
  }

  if (i2c_master_recv(toto_je_fuj_client, &val, 1) != 1) {//konflikt todo
    pr_devinit(KERN_ERR PFX "chyba read z mcp\n");
    return 0;
  }
  return val;
}

static void srv1_mcp23017_get_keycode(void) {
  unsigned char row = 0, column;

//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: called\n");
//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: toto_je_fuj_client = %x\n", toto_je_fuj_client);
  column = get_register(MCP_GPIOB);
//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: bp1\n");
  column = (~column) & 0xF0;
  set_register(MCP_GPINTENB,0x00);
//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: bp2\n");
  if (column) {
    set_register(MCP_GPIOB,0x0F);
    set_register(MCP_IODIRB,0x0F);
    row = get_register(MCP_GPIOB);
    row = (~row) & 0x0F;
    if (!row)
      column = 0;
  }
//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: bp3\n");
  set_register(MCP_IODIRB,0xF0);
  set_register(MCP_GPIOB,0xF0);
//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: bp4\n");
  set_register(MCP_DEFVALB,0xF0 & get_register(MCP_GPIOB));
  set_register(MCP_GPINTENB,0xF0);
//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: bp5\n");

  // TODO atomic
  if (mcp23017_bufflen < 8) {
    mcp23017_buffer[mcp23017_bufflen] = row | column;
    mcp23017_bufflen++;
  }
//  printk("srv1-mcp23017: srv1_mcp23017_get_keycode: row | column = %x\n", row | column);
}

//irq
static irqreturn_t srv1_mcp23017_handle_interrupt(int irq, void *data) {
  mcp23017_should_wake = 1;
  wake_up(&toto_je_fuj_wq);
  return IRQ_HANDLED;
}

/**
 *	srv1_mcp23017_write - debug only
 *
 *	Write directly to the output buffer
 */
static ssize_t srv1_mcp23017_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
//  unsigned int mcp23017 = iminor(file->f_path.dentry->d_inode);
  ssize_t ret = 0;
  while (ret < count) {
    char byte;
    int user_ret = get_user(byte, buf + ret++);
    if (user_ret)
      return user_ret;

    // TODO atomic, delete this (who wants to write to keyboard device??)
    if (mcp23017_bufflen < 8) {
      mcp23017_buffer[mcp23017_bufflen] = byte;
      printk("srv1-mcp23017: srv1_mcp23017_write: byte = %c\n", byte);
      mcp23017_bufflen++;
    }
  }
  printk("srv1-mcp23017: srv1_mcp23017_write: ret = %i\n", (int) ret);
  mcp23017_should_wake = 1;
  wake_up(&toto_je_fuj_wq);
  return ret;
}


/**
 *	srv1_mcp23017_open - claim the specified mcp23017
 *
 *	Grab the specified mcp23017 if it's available and keep track of how many times
 *	we've been opened (see close() below).  We allow multiple people to open
 *	at the same time as there's no real limitation in the hardware for reading
 *	from different processes.  Plus this way you can have one app do the write
 *	and management while quickly monitoring from another by doing:
 *		$ cat /dev/mcp230178
 */
static int srv1_mcp23017_open(struct inode *ino, struct file *file) {
  struct group_data *group_data = container_of(ino->i_cdev, struct group_data, cdev);
  unsigned int mcp23017 = iminor(ino);
  struct mcp23017_data *mcp23017_data = &group_data->mcp23017[mcp23017 - group_data->mcp23017_range->start];

  if (mcp23017 != 0)
    return -ENODEV;

  set_register(MCP_DEFVALB,0xF0 & get_register(MCP_GPIOB));
  set_register(MCP_GPINTENB,0xF0);

  atomic_inc(&mcp23017_data->open_count);

  return 0;
}

/**
 *	srv1_mcp23017_close - release the specified mcp23017
 *
 *	Do not actually free the specified mcp23017 until the last person has closed.
 *	We claim/release here rather than during probe() so that people can swap
 *	between drivers on the fly during runtime without having to load/unload
 *	kernel modules.
 *
 *	HA HA HA
 */
static int srv1_mcp23017_release(struct inode *ino, struct file *file) {
  struct group_data *group_data = container_of(ino->i_cdev, struct group_data, cdev);
  unsigned int mcp23017 = iminor(ino);
  struct mcp23017_data *mcp23017_data = &group_data->mcp23017[mcp23017 - group_data->mcp23017_range->start];



  /* do not free until last consumer has closed */
  if (!atomic_dec_and_test(&mcp23017_data->open_count)) {
    stamp("mcp23017 still in use -- not freeing");
  }
  else {
    set_register(MCP_GPINTENB,0x00);
  }

  return 0;
}

static struct class *srv1_mcp23017_class;

static struct file_operations srv1_mcp23017_fops = {
  .owner    = THIS_MODULE,
  .read     = srv1_mcp23017_read,
  .write    = srv1_mcp23017_write,
  .open     = srv1_mcp23017_open,
  .release  = srv1_mcp23017_release,
};

/**
 *	srv1_mcp23017_probe - setup the range of mcp23017s
 *
 *	Create a character device for the range of mcp23017s and have the minor be
 *	used to specify the mcp23017.
 */
static int __devinit srv1_mcp23017_probe(struct platform_device *pdev) {
  int ret;
  struct group_data *group_data;
  struct resource *mcp23017_range = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
  int mcp23017, mcp23017_max = mcp23017_range->end - mcp23017_range->start + 1;

  printk("srv1-mcp23017: srv1_mcp23017_probe: init\n");


  group_data = kzalloc(sizeof(*group_data) + sizeof(struct mcp23017_data) * mcp23017_max, GFP_KERNEL);
  if (!group_data)
    return -ENOMEM;
  group_data->mcp23017_range = mcp23017_range;
  group_data->mcp23017 = (void *)group_data + sizeof(*group_data);
  platform_set_drvdata(pdev, group_data);

  ret = alloc_chrdev_region(&group_data->dev_node, mcp23017_range->start, mcp23017_max, "mcp23017");
  if (ret) {
    pr_devinit(KERN_ERR PFX "unable to get a char device\n");
    kfree(group_data);
    return ret;
  }

  cdev_init(&group_data->cdev, &srv1_mcp23017_fops);
  group_data->cdev.owner = THIS_MODULE;

  ret = cdev_add(&group_data->cdev, group_data->dev_node, mcp23017_max);
  if (ret) {
    pr_devinit(KERN_ERR PFX "unable to register char device\n");
    kfree(group_data);
    unregister_chrdev_region(group_data->dev_node, mcp23017_max);
    return ret;
  }

  device_init_wakeup(&pdev->dev, 1);
  init_waitqueue_head(&toto_je_fuj_wq);
  ret = request_irq(MCP_INT_PIN_IRQ, srv1_mcp23017_handle_interrupt, IRQF_TRIGGER_FALLING, "srv1-mcp23017-irq", NULL);
  if (ret)
    printk("srv1-mcp23017: failed to request irq (%i)\n", ret);

  for (mcp23017 = mcp23017_range->start; mcp23017 <= mcp23017_range->end; ++mcp23017)
    device_create(srv1_mcp23017_class, &pdev->dev, group_data->dev_node + mcp23017,
	NULL, "kbd%i", mcp23017);

  pr_devinit(KERN_INFO PFX "now handling %i mcp23017s: %i - %i\n",
      mcp23017_max, mcp23017_range->start, mcp23017_range->end);

  return 0;
}

/**
 *	srv1_mcp23017_remove - break down the range of mcp23017s
 *
 *	Release the character device and related pieces for this range of mcp23017s.
 */
static int __devexit srv1_mcp23017_remove(struct platform_device *pdev) {
  struct group_data *group_data = platform_get_drvdata(pdev);
  struct resource *mcp23017_range = group_data->mcp23017_range;
  int mcp23017;

  gpio_free(MCP_INT_PIN);

  for (mcp23017 = mcp23017_range->start; mcp23017 <= mcp23017_range->end; ++mcp23017)
    device_destroy(srv1_mcp23017_class, group_data->dev_node + mcp23017);

  cdev_del(&group_data->cdev);
  unregister_chrdev_region(group_data->dev_node, 1);

  kfree(group_data);

  return 0;
}


struct platform_driver srv1_mcp23017_device_driver = {
  .probe   = srv1_mcp23017_probe,
  .remove  = __devexit_p(srv1_mcp23017_remove),
  .driver  = {
    .name = DRIVER_NAME,
  }
};

/**
 *	srv1_mcp23017_init - setup our odometry device driver
 *
 *	Create one odometry class for the entire driver
 */
static int __init srv1_mcp23017_init(void) {
  srv1_mcp23017_class = class_create(THIS_MODULE, DRIVER_NAME);
  if (IS_ERR(srv1_mcp23017_class)) {
    pr_init(KERN_ERR PFX "Unable to create mcp23017 class\n");
    return PTR_ERR(srv1_mcp23017_class);
  }

  i2c_add_driver(&srv1_mcp23017_driver);
  printk("srv1-mcp23017: i2c_driver_added\n");
  set_register(MCP_IOCON, 0x60);
  set_register(MCP_IODIRB,0xF0);
  set_register(MCP_GPPUB,0xFF);
  set_register(MCP_GPIOB,0xF0);
  set_register(MCP_INTCONB,0xF0);
  set_register(MCP_DEFVALB,0xF0);
  printk("srv1-mcp23017: toto_je_fuj_client = %x\n", (int) toto_je_fuj_client);

  gpio_request(MCP_INT_PIN, DRIVER_NAME);
  gpio_direction_input(MCP_INT_PIN);

  return platform_driver_register(&srv1_mcp23017_device_driver);
}
module_init(srv1_mcp23017_init);

static void __exit srv1_mcp23017_exit(void) {
  set_register(MCP_GPINTENB, 0x00);
  set_register(MCP_IODIRB, 0x00);

  class_destroy(srv1_mcp23017_class);
  i2c_del_driver(&srv1_mcp23017_driver);

  platform_driver_unregister(&srv1_mcp23017_device_driver);
}
module_exit(srv1_mcp23017_exit);

MODULE_AUTHOR("Jan Hadrava <jhadrava@gmail.com>");
MODULE_DESCRIPTION("Matrix keyborad driver mcp23017");
MODULE_LICENSE("GPL");
