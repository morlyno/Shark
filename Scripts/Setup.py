
import CheckPython
CheckPython.ValidatePackages()

import Vulkan
import Dotnet
from GenerateProjects import generateProject

import os
import subprocess

import colorama
from colorama import Style
from colorama import Back
from colorama import Fore

colorama.init()

def setup():

    os.chdir("./../")

    print(f"{Style.BRIGHT}{Back.GREEN}Setting SHARK_DIR to {os.getcwd()}{Style.RESET_ALL}")
    subprocess.call(["setx", "SHARK_DIR", os.getcwd()])
    os.environ['SHARK_DIR'] = os.getcwd()

    if not Vulkan.ValidateVulkanSDK():
        exit()

    Vulkan.ValidateVulkanDebugLibs()

    if not Dotnet.ValidateDotnet():
        exit()

    subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

    generateProject(options=["--with-vulkan"])

if __name__ == "__main__":
    setup()
