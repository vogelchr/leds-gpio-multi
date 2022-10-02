# LEDs using multiple GPIOs

If an LED is not simply connected to a GPIO, but needs certain GPIO patterns
to be be present on multiple GPIOs so that it lights up, or gets extinguished.

## Example

As an example, assume you have one of the typical dual-color LEDs, where two
light-emitting dies are connected in opposing polarity in one chip housing
connected to two GPIOs (which can source and sink current) like shown in the
schematic below:

```
                         //
                    +---|>|---+ (green)
                    |         |
   +---[Resistor]---+         +-------+
   |                |    //   |       |
   |                +---|<|---+ (red) |
   |                                  |
   O (GPIO A)                (GPIO B) O
```

The LEDs will light up under the following conditions:

| GPIO A | GPIO B | green LED | red LED |
|--------|--------|-----------|---------|
| L      | L      | off       | off     |
| H      | L      | **on**    | off     |
| L      | H      | off       | **on**  |
| H      | H      | off       | off     |


Obviously, the two LEDs have interdependencies, e.g. they cannot made lit
up at the same time (assuming steady levels on each pins).

Using this kernel module, one can at least get two somehow independent
LEDs from them, with the restriction that only the last status change will
be reflected by each LED.

## Configuration

In your device-tree choose the set of GPIOs that will be used to toggle the
LEDs, using the gpios property:

```
   leds_gpio_multi {
            compatible = "leds-gpio-multi";
            status = "okay";
            gpios = <&pio 2 7 GPIO_ACTIVE_HIGH>, <&pio 2 8 GPIO_ACTIVE_HIGH>;
   (...)
```

These GPIOs are corresponding to the bits in the properties you will configure
for each individual LEDs within the leds_gpio_multi group. For how to refer to
the GPIOs, see the documentation for your SOC and its device-tree configuration.

Each LED will get a configuration property `mask-bit-on-off` consisting of three
numbers. A mask that defines which of the bits above will be toggled, one bit-field
that defines which of the of the masked GPIOs will be turned on or off when turning
on the LED, and one bit-field that defines which of the masked GPIOs will
be turned on or off when turning off the LED.

```
   (...)
            led0 {
                  label = "button_red";
                  /*
                  *  Gpios:  PC8 PC7
                  *     on:   L   H    mask=0x03 bits=0x01
                  *    off:   L   L    mask=0x03 bits=0x00
                  */
                  mask-bit-on-off = <0x03 0x01 0x00>;
            };
   (...)
```

Here, the mask is 0x03, so the first and second GPIO are used (i.e. all of the
above) and the on-value is 0x01. Therefore, when the LED will be turned on (i.e.
its brightness property will be set to 1), the first GPIO in the list above will
be set to *HIGH* (because the bit `1<<0` is set), and the second GPIO in the list above will
be set to *LOW* (because the bit `1<<1` is cleared).

When turning off the LED (i.g. its brighness property will be set to 0) both GPIOs will
be set to *LOW* because all bits are zero.


