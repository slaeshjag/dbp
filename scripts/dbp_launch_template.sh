#!/bin/bash

# Wrapper script that simulates a binary of the desired program inside $PATH
# For use with the pyra-hspkg system

# anything surrounded by !! is replaced by actual values by pyra-hspkgd when
# the special folder is populated

pyra-hspkg-run !!package_path!! --sub-app !!package_binary!! --args "$@"
