import os
import subprocess

# Compile core shaders
current_dir = os.path.dirname(os.path.realpath(__file__))
compile_shader_script_path = current_dir + "/graphics/script/compile_shader.py"
core_shader_dir = current_dir + "/graphics/shaders"

for file in os.listdir(core_shader_dir):
	if file.endswith(".vert") or file.endswith(".frag") or file.endswith(".comp"):
		subprocess.check_call(["py", "-3", compile_shader_script_path, os.path.join(core_shader_dir, file)], stderr=subprocess.STDOUT)

# Build core
os.chdir(current_dir)

build_dir = "./build"

if not os.path.isdir(build_dir):
    os.mkdir(build_dir)

os.chdir(build_dir)

# Configure
subprocess.check_call(["cmake.exe", "../"], stderr=subprocess.STDOUT)

# Build
subprocess.check_call(["cmake.exe", "--build", "."], stderr=subprocess.STDOUT)
