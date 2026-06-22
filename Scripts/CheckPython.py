import importlib
import subprocess
import sys

def ValidatePackage(package:str):
    try:
        importlib.import_module(package)
    except:
        print(f"Installing package {package}...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])

def ValidatePackages():
    ValidatePackage("colorama")
