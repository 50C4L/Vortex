import os
import subprocess

build_dir = "./build"

if not os.path.isdir(build_dir):
    os.mkdir(build_dir)

os.chdir(build_dir)

# Configure
subprocess.check_call(["cmake.exe", "../"], stderr=subprocess.STDOUT)

# Build
subprocess.check_call(["cmake.exe", "--build", "."], stderr=subprocess.STDOUT)
