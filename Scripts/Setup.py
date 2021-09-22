
import os
import subprocess
import platform

os.chdir("./../")

from SetupPremake import Validate
premakeInstalled = Validate()

print("\nUpdating Submodules")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

if premakeInstalled:
    if platform.system() == "Windows":
        print("\nRunning premake...")
        subprocess.call([os.path.abspath("./Scripts/Win-GenerateProjects.bat"), "nopause"])

    print("\nSetup Comlete")
else:
    print("\nPremake is required to generate projects")