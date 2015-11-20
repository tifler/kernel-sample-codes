/*
 * sysfs simple guide
 *
 * Copyright (c) 2015 Youngdo, Lee <nungdo@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define	DEBUG
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#define	MODULE_NAME			"sysfs_sample"
#define	MAX_COUNT			((int)((PAGE_SIZE / 9) - 1))
#define ATTR_PTR(_name)                 (&dev_attr_##_name)

/*****************************************************************************/

struct context {
	struct mutex lock;
	int count;
	unsigned int data;
};

/*****************************************************************************/

static int pseudo_read_device(struct context *ctx)
{
	static int last;

	ctx->data = last++;

	return 0;
}

/*****************************************************************************/

static ssize_t sysfs_sample_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int i;
	int ret;
	ssize_t size = (ssize_t)0;
	ssize_t len = (ssize_t)0;
	struct context *ctx = dev_get_drvdata(dev);

	pr_debug(MODULE_NAME ": %s\n", __func__);

	mutex_lock(&ctx->lock);
	for (i = 0; i < ctx->count; i++) {
		ret = pseudo_read_device(ctx);
		if (ret < 0)
			break;

		/* 
		 * buf is a PAGE_SIZE buffer allocated by sysfs.
		 * so you should return size less then PAGE_SIZE.
		 */
		size = sprintf(buf, "%08x ", ctx->data);
		len += size;
		buf += size;
	}
	mutex_unlock(&ctx->lock);

	if (i > 0) {
		len += sprintf(buf, "\n");
	}

	pr_debug(MODULE_NAME ": len = %d\n", (int)len);

	BUG_ON(len > PAGE_SIZE);

	return len;
}

static ssize_t sysfs_sample_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int req;
	struct context *ctx = dev_get_drvdata(dev);

	pr_debug(MODULE_NAME ": %s\n", __func__);

	kstrtoint(buf, 0, &req);

	if (req > MAX_COUNT) {
		pr_err(MODULE_NAME ": maximum count is %d\n", MAX_COUNT);
		return -ERANGE;
	}

	mutex_lock(&ctx->lock);
	ctx->count = req;
	mutex_unlock(&ctx->lock);

	pr_debug("count = %d\n", ctx->count);

	return count;
}

static DEVICE_ATTR(data, S_IRUGO | S_IWUGO,
		   sysfs_sample_show, sysfs_sample_store);

static struct device_attribute *attrs[] = {
	ATTR_PTR(data),
};

/*****************************************************************************/

static int sysfs_sample_probe(struct platform_device *pdev)
{
	int i;
	int ret;
	struct context *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);

	mutex_init(&ctx->lock);
	ctx->count = 1;	/* default count */

	platform_set_drvdata(pdev, ctx);

	for (i = 0; i < ARRAY_SIZE(attrs); i++) {
		ret = device_create_file(&pdev->dev, attrs[i]);
		if (ret) {
			pr_err("device_create_file failed.");
			for (--i; i >= 0; i--)
				device_remove_file(&pdev->dev, attrs[i]);
			break;
		}
	}

	return 0;
}

static int sysfs_sample_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attrs); i++)
		device_remove_file(&pdev->dev, attrs[i]);

	return 0;
}

/*****************************************************************************/

static struct platform_device sysfs_sample_device = {
	.name = MODULE_NAME,
	.id = PLATFORM_DEVID_NONE,
};

static struct platform_driver sysfs_sample_driver = {
	.probe = sysfs_sample_probe,
	.remove = sysfs_sample_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static void sysfs_sample_dummy_release(struct device *dev)
{
	pr_debug(MODULE_NAME ": %s\n", __func__);
}

static int __init sysfs_sample_init(void)
{
	int ret;

	ret = platform_driver_register(&sysfs_sample_driver);
	if (ret) {
		pr_err(MODULE_NAME ": register failed.(ret=%d)\n", ret);
		return ret;
	}
	pr_debug(MODULE_NAME ": driver registered.\n");

	ret = platform_device_register(&sysfs_sample_device);
	if (ret) {
		pr_err(MODULE_NAME ": register failed.(ret=%d)\n", ret);
		return ret;
	}
	sysfs_sample_device.dev.release = sysfs_sample_dummy_release;
	pr_debug(MODULE_NAME ": device registered.\n");

	return ret;
}

static void __exit sysfs_sample_exit(void)
{
	platform_device_unregister(&sysfs_sample_device);
	pr_debug(MODULE_NAME ": device unregistered.\n");
	platform_driver_unregister(&sysfs_sample_driver);
	pr_debug(MODULE_NAME ": driver unregistered.\n");
}

module_init(sysfs_sample_init);
module_exit(sysfs_sample_exit);

MODULE_AUTHOR("Youngdo, Lee <nungdo@gmail.com>");
MODULE_DESCRIPTION("Sysfs simple guide");
MODULE_LICENSE("GPL");
