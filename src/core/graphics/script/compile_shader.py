import os
import subprocess
import argparse

parser = argparse.ArgumentParser(description="Compile a shader using glslc")
parser.add_argument("input", help="The input file fuill path")
args=parser.parse_args()

dir_path = os.path.dirname(os.path.realpath(__file__))
glslc_path = "../bin/glslc.exe"

os.chdir(dir_path)

input_file = args.input
input_file_name = os.path.basename(input_file)
input_file_dir = os.path.dirname(input_file)
compiled_file_dir = os.path.join(input_file_dir, "compiled")

if not os.path.isdir(compiled_file_dir):
    os.mkdir(compiled_file_dir)

output_file_path = os.path.join(compiled_file_dir, input_file_name + ".spv")

# Compile
subprocess.check_call([glslc_path, input_file, "-o", output_file_path], stderr=subprocess.STDOUT)