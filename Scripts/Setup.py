
import os
import sys
import subprocess

from SetupPremake import Premake
from SetupMono import Mono

def Setup():
    os.chdir("./../")

    Mono.Validate()
    premakeInstalled = Premake.Validate()

    print("\nUpdate Submodules...")
    subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

    if (premakeInstalled):
        print("\nGenerate Projects...")
        targetVersion = input("Version: ").lower().strip().removesuffix('\n')
        subprocess.call([f"{Premake.Directory}/premake5.exe", targetVersion])

if __name__ == "__main__":
    Setup()
