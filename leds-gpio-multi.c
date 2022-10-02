#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>

struct leds_gpio_multi_led {
	struct led_classdev cdev;
	u32 gpio_mask;
	u32 gpio_on_bits;
	u32 gpio_off_bits;
};

struct leds_gpio_multi_priv {
	struct gpio_descs *gpios;
	struct leds_gpio_multi_led *leds;

	int can_sleep;
};

static int leds_gpio_multi_set(struct led_classdev *led_cdev,
			       enum led_brightness brightness)
{
	struct leds_gpio_multi_led *led =
		container_of(led_cdev, struct leds_gpio_multi_led, cdev);
	struct leds_gpio_multi_priv *priv =
		dev_get_drvdata(led_cdev->dev->parent);
	u32 gpio_bits;
	unsigned int i = 0;

	if (brightness)
		gpio_bits = led->gpio_on_bits;
	else
		gpio_bits = led->gpio_off_bits;

	for (i = 0; i < priv->gpios->ndescs; i++) {
		unsigned int level = !!(gpio_bits & (1 << i));
		struct gpio_desc *gpio = priv->gpios->desc[i];

		/* led which is currently changing doesn't use this particular IO */
		if (!(led->gpio_mask & (1 << i)))
			continue;

		if (priv->can_sleep)
			gpiod_set_value_cansleep(gpio, level);
		else
			gpiod_set_value(gpio, level);
	};

	return 0;
};

/* we handle both in the generic version above, but led_cdev.rightness_set needs
   a different function signature */
static void leds_gpio_multi_set_not_blocking(struct led_classdev *led_cdev,
					     enum led_brightness brightness)
{
	leds_gpio_multi_set(led_cdev, brightness);
}

static int leds_gpio_multi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child = NULL;
	struct leds_gpio_multi_priv *priv;

	int retval;
	unsigned int count, lednum, gpionum;

	count = device_get_child_node_count(dev);

	/* reserve space for our private data structure and count LEDs */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dev_set_drvdata(dev, priv);

	priv->leds = devm_kcalloc(
		dev, count, sizeof(struct leds_gpio_multi_led), GFP_KERNEL);
	if (!priv->leds)
		return -ENOMEM;

	/* get GPIOs */
	priv->gpios = devm_gpiod_get_array(dev, NULL, GPIOD_OUT_LOW);
	if (IS_ERR(priv->gpios))
		return PTR_ERR(priv->gpios);

	if (priv->gpios->ndescs == 0) {
		dev_err(dev, "No gpios specified!");
		return -EINVAL;
	}

	if (priv->gpios->ndescs > 32) {
		dev_err(dev, "Too many gpios (%d, max is 32) specified!",
			count);
		return -EINVAL;
	}

	/* check that all GPIOs can sleep (or none of them can) */
	priv->can_sleep = gpiod_cansleep(priv->gpios->desc[0]);
	for (gpionum = 1; gpionum < priv->gpios->ndescs; gpionum++) {
		if (priv->can_sleep != gpiod_cansleep(priv->gpios->desc[1])) {
			dev_err(dev,
				"Inconsistent gpio can_sleep properties for gpio #%d\n",
				gpionum);
			return -EINVAL;
		}
	}

	/* instantiate all the LEDs */
	lednum = 0;
	device_for_each_child_node(dev, child) {
		uint32_t mask_bit_on_off[3];
		struct leds_gpio_multi_led *led = &priv->leds[lednum];
		struct led_init_data init_data = { .fwnode = child };

		/* what GPIOs to toggle? */

		retval = fwnode_property_read_u32_array(
			child, "mask-bit-on-off", mask_bit_on_off, 3);
		if (retval) {
			dev_err(dev,
				"Cannot read mask-bit-on-off properrty for led#%d, err=%d.",
				lednum, retval);
			goto err_node_put;
		}

		led->gpio_mask = mask_bit_on_off[0];
		led->gpio_on_bits = mask_bit_on_off[1];
		led->gpio_off_bits = mask_bit_on_off[2];

		/* generic led properties */
		led->cdev.max_brightness = 1;

		if (priv->can_sleep)
			led->cdev.brightness_set_blocking =
				&leds_gpio_multi_set;
		else
			led->cdev.brightness_set =
				&leds_gpio_multi_set_not_blocking;

		retval = devm_led_classdev_register_ext(dev, &led->cdev,
							&init_data);
		if (retval) {
			dev_err(dev,
				"Cannot register classdev for led#%d, err=%d.",
				lednum, retval);
			goto err_node_put;
		}
		lednum++;
	};

	return 0;

err_node_put:
	fwnode_handle_put(child);
	return retval;
};

static const struct of_device_id leds_gpio_multi_of_match[] = {
	{ .compatible = "leds-gpio-multi" },
	{},
};
MODULE_DEVICE_TABLE(of, leds_gpio_multi_of_match);

static struct platform_driver leds_gpio_multi_of_driver = {
	.driver = {
		.name = "leds-gpio-multi-drvr",
		.of_match_table = of_match_ptr(leds_gpio_multi_of_match),
	},
	.probe = leds_gpio_multi_probe
};
module_platform_driver(leds_gpio_multi_of_driver);

MODULE_AUTHOR("Christian Vogel <vogelchr@vogel.cx>");
MODULE_DESCRIPTION("LED Driver for dual-color LEDs");
MODULE_LICENSE("GPL");