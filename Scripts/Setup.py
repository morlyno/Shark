
import os
import sys
import subprocess

import SetupPremake as Premake

os.chdir("./../")

print("\nUpdating Submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

premakeInstalled = Premake.Validate()
if (premakeInstalled):
    print("\nGenerating Project Files")
    Premake.GenerateProject()
else:
    print("Premake is required to generate project files")
