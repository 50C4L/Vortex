import os
import subprocess

# Compile core shaders
# @todo: This should be an optional step configurable via a command line argument
current_dir = os.path.dirname(os.path.realpath(__file__))
compile_shader_script_path = current_dir + "/src/core/graphics/script/compile_shader.py"
core_shader_dir = current_dir + "/src/core/graphics/shaders"
ui_shader_dir = current_dir + "/src/core/ui/shaders"
game_shader_dir = current_dir + "/src/shaders"

for file in os.listdir(core_shader_dir):
	if file.endswith(".vert") or file.endswith(".frag") or file.endswith(".comp"):
		subprocess.check_call(["py", "-3", compile_shader_script_path, os.path.join(core_shader_dir, file)], stderr=subprocess.STDOUT)

for file in os.listdir(ui_shader_dir):
	if file.endswith(".vert") or file.endswith(".frag") or file.endswith(".comp"):
		subprocess.check_call(["py", "-3", compile_shader_script_path, os.path.join(ui_shader_dir, file)], stderr=subprocess.STDOUT)

for file in os.listdir(game_shader_dir):
	if file.endswith(".vert") or file.endswith(".frag") or file.endswith(".comp"):
		subprocess.check_call(["py", "-3", compile_shader_script_path, os.path.join(game_shader_dir, file)], stderr=subprocess.STDOUT)

# Build project
build_dir = "./build"

if not os.path.isdir(build_dir):
    os.mkdir(build_dir)

os.chdir(build_dir)

# Configure
subprocess.check_call(["cmake.exe", "../"], stderr=subprocess.STDOUT)

# Build
subprocess.check_call(["cmake.exe", "--build", "."], stderr=subprocess.STDOUT)
