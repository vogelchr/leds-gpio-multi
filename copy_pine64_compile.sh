#!/bin/sh

make clean
rsync -av . pine64:leds-gpio-multi
ssh pine64 cd leds-gpio-multi \; make clean \; make
