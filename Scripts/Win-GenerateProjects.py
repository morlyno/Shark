import os
import subprocess

os.chdir("./../")

print("Generating Visual Studio 2022 solution.")
subprocess.call(["dependencies/premake/bin/premake5.exe", "vs2022"])
os.chdir("Shark-Editor/SandboxProject")
subprocess.call(["../../dependencies/premake/bin/premake5.exe", "vs2022"])