
import os
import requests
import zipfile
from pathlib import Path
import winreg
import subprocess
import pathlib

def SetEnvVar(key:str, val:str):
    val = val.replace("/", "\\")
    os.environ[key] = val

def GetDownloadsFolderPath():
    subkey = r'SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders'
    guid = '{374DE290-123F-4565-9164-39C4925E467B}'
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, subkey) as key:
        return winreg.QueryValueEx(key, guid)[0]


def DownloadFile(url : str, filepath : str):
    filepath = os.path.abspath(filepath)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'wb') as file:
        content = requests.get(url, stream=True).content
        file.write(content)

def UnzipFiles(filepath:Path, deleteZip:bool):
    zipFilePath = os.path.abspath(filepath)
    zipFileLocation = os.path.dirname(zipFilePath)
    with zipfile.ZipFile(filepath, 'r') as zip:
        zip.extractall(zipFileLocation)
    if (deleteZip):
        os.remove(zipFilePath)

def UnzipFile(filepath:Path, fileToUnzip, deleteZip:bool):
    zipFilePath = os.path.abspath(filepath)
    zipFileLocation = os.path.dirname(zipFilePath)
    with zipfile.ZipFile(filepath, 'r') as zip:
        zip.extract(fileToUnzip, zipFileLocation)
    if (deleteZip):
        os.remove(zipFilePath)
