import sys
import os
from pathlib import Path
import Utils

premakeVersion = "5.0.0-alpha16"
premakeZipUrls = f"https://github.com/premake/premake-core/releases/download/v{premakeVersion}/premake-{premakeVersion}-windows.zip"
premakeLicenseUrl = "https://raw.githubusercontent.com/premake/premake-core/master/LICENSE.txt"
premakeDirectory = "./dependencies/premake/bin"

def InstallPremake():
    validReply = False
    while validReply:
        reply = str(input(f"Preamek not found would you like to install Premake {premakeVersion}? [Y/N]")).lower().strip()[:1]
        if reply == 'n':
            return False
        validReply = (reply == 'y')
    
    preamkePath = f"{premakeDirectory}/premake-{premakeVersion}-windows.zip"
    print(f"Downloading {premakeZipUrls} to {preamkePath}")
    Utils.DownloadFile(premakeZipUrls, preamkePath)
    print(f"Extracting {preamkePath}")
    Utils.UnzipFile(preamkePath, deleteZipFile=True)
    print(f"Premake {premakeVersion} has been dpwnloaded to '{premakeDirectory}'")

    premakeLicensePath = f"{premakeDirectory}/LICENSE.txt"
    print(f"Downloading {premakeLicenseUrl} to {premakeLicensePath}")
    Utils.DownloadFile(premakeLicenseUrl, premakeLicensePath)
    print(f"Premake License file has been downloaded to '{premakeDirectory}'")

    return True

def Validate():
    premakeExe = Path(f"{premakeDirectory}/premake5.exe")
    if (not premakeExe.exists()):
        installed = InstallPremake()
        if (not installed):
            print("Premake not installed")
    
    print(f"Correct Premake version installed at {premakeDirectory}/{premakeVersion}")
    return True
