
from pathlib import Path

import utils
import subprocess

PremakeVersion = "5.0.0-beta1"
PremakeDirectory = f"./dependencies/premake/bin/{PremakeVersion}"

PremakeUrl = f"https://github.com/premake/premake-core/releases/download/v{PremakeVersion}/premake-{PremakeVersion}-windows.zip"
PremakeLicenseUrl = "https://raw.githubusercontent.com/premake/premake-core/master/LICENSE.txt"

def InstallPremake():
    zipPath = f"{PremakeDirectory}/premake-{PremakeVersion}-windows.zip"
    print("Downloading Premake")
    utils.DownloadFile(PremakeUrl, zipPath)

    print("Extracting Premake form zip")
    utils.UnzipFile(zipPath, True)

    print("Download Premake License")
    licensePath = f"{PremakeDirectory}/LICENSE.txt"
    utils.DownloadFile(PremakeLicenseUrl, licensePath)
    return True


def Validate():
    premakeExePath = Path(f"{PremakeDirectory}/premake5.exe")
    if (not premakeExePath.exists()):
        installed = InstallPremake()
        if (not installed):
            print("Premake not installed")
            return False
    print(f"Correct Version of Premake Installed at {PremakeDirectory}/premake5.exe")
    return True

def GenerateProject():
    targetVersion = input("Version: ").lower().strip().removesuffix('\n')
    subprocess.call([f"{PremakeDirectory}/premake5.exe", targetVersion])
