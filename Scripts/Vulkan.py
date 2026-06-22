
import os
import re
from pathlib import Path

from colorama import Style
from colorama import Back
from colorama import Fore


VULKAN_SDK = os.environ.get("VULKAN_SDK")
REQUIRED_VULKAN_VERSION = "1.3"

def ExtractVersion(path):
    match = re.search(r'\d+\.\d+', path)
    if match:
        return match.group(0)
    return None

def ValidateVulkanSDK():
    if VULKAN_SDK is None:
        print(f"{Style.BRIGHT}{Back.RED}Vulkan SDK is not installed!{Style.RESET_ALL}")
        return False
    
    version = ExtractVersion(VULKAN_SDK)
    if version is None:
        print(f"{Style.BRIGHT}{Back.YELLOW}Could not determine Vulkan SDK version! (Shark requires at least version 1.3){Style.RESET_ALL}")
        return False
    elif float(version) < 1.3:
        print(f"{Style.BRIGHT}{Back.RED}Installed Vulkan SDK is to old! (Required is 1.3, install is {version}){Style.RESET_ALL}")
        return False
    
    print(f"{Style.BRIGHT}{Back.GREEN}Correkt Vulkan SDK located at {VULKAN_SDK}{Style.RESET_ALL}")
    return True

def ValidateVulkanDebugLibs():
    dxc = Path(f"{VULKAN_SDK}/Lib/dxcompilerd.lib")
    if not dxc.exists():
        print(f"{Style.BRIGHT}{Back.YELLOW}Warning: No Vulkan SDK debug libraries found. (checked {dxc})")
        print(f"{Back.RED}Debug builds are not possible.{Style.RESET_ALL}")
        return False
    return True