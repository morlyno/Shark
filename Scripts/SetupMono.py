
import subprocess
import os
from pathlib import Path
import requests
import re

import utils

ProgramFiles = os.getenv("PROGRAMFILES")

class Mono:
    Url = "https://download.mono-project.com/archive/6.12.0/windows-installer/mono-6.12.0.107-x64-0.msi"
    Installer = "mono-6.12.0.107-x64-0.msi"
    MonoInstallPath = f"{ProgramFiles}/Mono"
    EnvKey = "MONO_PROJECT"

    @classmethod
    def SetEnvVar(cls):
        print("Setting Mono Enviorment Variable...")
        utils.SetEnvVar(cls.EnvKey, cls.MonoInstallPath)


    @classmethod
    def Install(cls):
        if (not os.path.exists(cls.Installer)):
            print("Downloading Mono...")
            utils.DownloadFile(cls.Url, cls.Installer)

        print("Execution Installer...")
        subprocess.call(cls.Installer, shell=True)
        if (os.path.exists(cls.MonoInstallPath)): # Checks if mono got installed
            cls.SetEnvVar()
            os.remove(cls.Installer)
        

    @classmethod
    def Validate(cls):
        env = os.getenv(cls.EnvKey)
        if (env and os.path.exists(env)):
            print("Mono Correct installed")
            return True
            
        if (os.path.exists(cls.MonoInstallPath)):
            setEnv = True
            if (env):
                print("Found Mono Directory but Eviorment Variable is Incorrect")
                print(f"Want to reset Enviorment Variable {cls.EnvKey} from {env} to {cls.MonoInstallPath}?")
                respons = str(input("[Y/N]: ")).lower().strip()[:1]
                while (True):
                    if (respons == 'y'):
                        setEnv = True
                        break
                    if (respons == 'n'):
                        setEnv = False
                        break
            if (setEnv):
                cls.SetEnvVar()
                return True
            return False
        
        respons = input("Mono Directory not found want to install Mono? [Y/N]: ").lower().strip()[:1]
        while (True):
            if (respons == 'y'):
                return cls.Install()
            if (respons == 'n'):
                print("If you have Mono already installed but in different location set an Envioment Variable call MONO_PROJECT")
                return False


