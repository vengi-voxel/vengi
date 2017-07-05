#!/bin/sh
#
# helper bash script to generate gl files
# python3-wheezy.template must be installed
#
python3 flextGLgen.py -T sdl -D ../../src/modules/video/gl flextgl_profile.txt
