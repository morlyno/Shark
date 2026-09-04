import os
import subprocess

import colorama
from colorama import Style
from colorama import Back
from colorama import Fore

colorama.init()

def generateProject(*, targetDirectory = None, options = None):

    if options is None:
        options = [
            "--with-vulkan",
            # "--with-d3d12",
            # "--with-d3d11",
        ]

    if targetDirectory is not None:
        os.chdir(targetDirectory)

    version = input("Version [2022|2026(default)] ").strip() or "2026"
    if version not in ["2022", "2026"]:
        print(f"{Fore.RED}Invalid version '{version}'{Style.RESET_ALL}")
        exit()

    print(f"{Style.BRIGHT}{Back.GREEN}Generating Visual Studio {version} solution.{Style.RESET_ALL}")

    action = f"vs{version}"
    premakePath = os.path.abspath("dependencies/premake/bin/premake5.exe")
    subprocess.call([premakePath, action] + options)
    subprocess.call([premakePath, action, "--file=Shark-Editor/SandboxProject/premake5.lua"])

if __name__ == "__main__":
    generateProject(targetDirectory="./../")
