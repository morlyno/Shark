import os
import subprocess

import colorama
from colorama import Style
from colorama import Back
from colorama import Fore

colorama.init()
os.chdir("./../")

version = input("Version [2022|2026(default)] ").strip() or "2026"
if version not in ["2022", "2026"]:
    print(f"{Fore.RED}Invalid version '{version}'{Style.RESET_ALL}")
    exit()

print(f"{Style.BRIGHT}{Back.GREEN}Generating Visual Studio {version} solution.{Style.RESET_ALL}")

action = f"vs{version}"
premakePath = os.path.abspath("dependencies/premake/bin/premake5.exe")
subprocess.call([premakePath, action])
subprocess.call([premakePath, action, "--file=Shark-Editor/SandboxProject/premake5.lua"])
