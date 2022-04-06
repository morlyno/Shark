
from pathlib import Path

import Utility

class Premake:
    Version = "5.0.0-beta1"
    Directory = f"./dependencies/premake/bin/{Version}"

    Url = f"https://github.com/premake/premake-core/releases/download/v{Version}/premake-{Version}-windows.zip"
    LicenseUrl = "https://raw.githubusercontent.com/premake/premake-core/master/LICENSE.txt"

    @classmethod
    def Install(cls):
        zipPath = f"{cls.Directory}/premake-{cls.Version}-windows.zip"
        print("Downloading Premake")
        Utility.DownloadFile(cls.Url, zipPath)

        print("Extracting Premake form zip")
        Utility.UnzipFile(zipPath, True)

        print("Download Premake License")
        licensePath = f"{cls.Directory}/LICENSE.txt"
        Utility.DownloadFile(cls.LicenseUrl, licensePath)
        return True


    @classmethod
    def Validate(cls):
        premakeExePath = Path(f"{cls.Directory}/premake5.exe")
        if (not premakeExePath.exists()):
            installed = Premake.Install()
            if (not installed):
                print("Premake not installed")
                return False
        print(f"Correct Version of Premake Installed at {cls.Directory}/premake5.exe")
        return True

