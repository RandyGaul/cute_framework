@echo off
py single_header_packer.py --root ../../src/ --macro CUTE_FRAMEWORK --intro intro.txt --hdrs hdrs.txt --srcs srcs.txt > cute_framework.h