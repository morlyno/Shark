
import os
import requests
import zipfile
from pathlib import Path

def DownloadFile(url : str, filepath : str):
    filepath = os.path.abspath(filepath)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'wb') as file:
        content = requests.get(url, stream=True).content
        file.write(content)

def UnzipFile(filepath:Path, deleteZip:bool):
    zipFilePath = os.path.abspath(filepath)
    zipFileLocation = os.path.dirname(zipFilePath)
    with zipfile.ZipFile(filepath, 'r') as zip:
        zip.extractall(zipFileLocation)
    if (deleteZip):
        os.remove(zipFilePath)
