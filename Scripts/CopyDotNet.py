import sys
import os
import shutil

print("Running CopyDotNet.py")
if not (len(sys.argv) == 2 and (sys.argv[1] == "Debug" or sys.argv[1] == "Debug-AS" or sys.argv[1] == "Release")):
    print(f"Invalid argument: '{sys.argv[1] if len(sys.argv) > 1 else 'null'}'\nValid arguments are 'Debug', 'Debug-AS' or 'Release'\n")
    exit(1)

Config = sys.argv[1]
SharkDir = os.environ.get("SHARK_DIR")
DotNetDir = f"{SharkDir}/Shark-Editor/DotNet"
CoralDir = f"{SharkDir}/Shark/dependencies/Coral"
BuildDir = f"{CoralDir}/Build/{Config}"

if (not os.path.exists(DotNetDir)):
    os.makedirs(DotNetDir)

try:
    shutil.copyfile(f"{CoralDir}/Coral.Managed/Coral.Managed.runtimeconfig.json", f"{DotNetDir}/Coral.Managed.runtimeconfig.json")
    print(f"copied {CoralDir}/Coral.Managed/Coral.Managed.runtimeconfig.json -> {DotNetDir}/Coral.Managed.runtimeconfig.json")

    shutil.copyfile(f"{BuildDir}/Coral.Managed.deps.json", f"{DotNetDir}/Coral.Managed.deps.json")
    print(f"copied {BuildDir}/Coral.Managed.deps.json -> {DotNetDir}/Coral.Managed.deps.json")

    shutil.copyfile(f"{BuildDir}/Coral.Managed.dll", f"{DotNetDir}/Coral.Managed.dll")
    print(f"copied {BuildDir}/Coral.Managed.dll -> {DotNetDir}/Coral.Managed.dll")

    shutil.copyfile(f"{BuildDir}/Coral.Managed.pdb", f"{DotNetDir}/Coral.Managed.pdb")
    print(f"copied {BuildDir}/Coral.Managed.pdb -> {DotNetDir}/Coral.Managed.pdb")
except Exception as e:
    print(e)
