import os
import subprocess

import colorama
from colorama import Style
from colorama import Back

colorama.init()

os.chdir("./../")

print(f"{Style.BRIGHT}{Back.GREEN}Setting SHARK_DIR to {os.getcwd()}{Style.RESET_ALL}")
subprocess.call(["setx", "SHARK_DIR", os.getcwd()])
os.environ['SHARK_DIR'] = os.getcwd()

subprocess.call(["git", "submodule", "update", "--init", "--recursive"])


print(f"{Style.BRIGHT}{Back.GREEN}Generating Visual Studio 2022 solution.{Style.RESET_ALL}")
premakePath = os.path.abspath("dependencies/premake/bin/premake5.exe")
subprocess.call([premakePath, "vs2022"])
os.chdir("Shark-Editor/SandboxProject")
subprocess.call([premakePath, "vs2022"])