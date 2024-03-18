import os
import subprocess

# Compile shader
# compile_shader_script_path = "../src/core/graphics/script/compile_shader.py"
# subprocess.check_call(["py", "-3", compile_shader_script_path, "../src/shaders/vertex.glsl"], stderr=subprocess.STDOUT)

build_dir = "./build"

if not os.path.isdir(build_dir):
    os.mkdir(build_dir)

os.chdir(build_dir)

# Configure
subprocess.check_call(["cmake.exe", "../"], stderr=subprocess.STDOUT)

# Build
subprocess.check_call(["cmake.exe", "--build", "."], stderr=subprocess.STDOUT)
