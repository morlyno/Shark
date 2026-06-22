
import subprocess
import re

from colorama import Style
from colorama import Back
from colorama import Fore

def ExtractVersion(path):
    match = re.search(r'\d+\.\d+', path)
    if match:
        return match.group(0)
    return None

def ValidateDotnet():
    r = subprocess.run(["dotnet", "--version"], capture_output=True, text=True, check=False)

    if r.returncode != 0:
        print(f"{Style.BRIGHT}{Back.RED}Dotnet is not installed!{Style.RESET_ALL}")
        return False
    
    version = ExtractVersion(r.stdout.strip())

    if version is None:
        print(f"{Style.BRIGHT}{Back.YELLOW}Could not determine .Net SDK version! (Shark requires at least .Net 9){Style.RESET_ALL}")
    elif float(version) < 9.0:
        print(f"{Style.BRIGHT}{Back.RED}Installed .NET SDK is to old! (Required is .Net 9, install is {version}){Style.RESET_ALL}")
        return False
    
    
    print(f"{Style.BRIGHT}{Back.GREEN}Correkt .NET SDK located{Style.RESET_ALL}")
    return True
