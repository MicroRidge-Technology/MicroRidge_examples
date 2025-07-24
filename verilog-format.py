#!/usr/bin/python3

# This command is meant to be automatically run be pre-commit https://pre-commit.com/
# To install run the following commands:
#     pipx install pre-commit
#     pre-commit install
#     pre-commit run --all-files #optional, runs on all files

import urllib.request
import tarfile
import tempfile
import shutil
import os
import hashlib
import io
import sys
import subprocess
import zipfile
def get_os():
    return "win64.zip" if sys.platform.startswith("win") else "linux-static-x86_64.tar.gz"
def get_exe():
    return "verible/bin/verible-verilog-format" if "win" not in get_os() else "verible/verible-verilog-format.exe"
def get_verible_format():

    url = f"https://github.com/chipsalliance/verible/releases/download/v0.0-3756-gda9a0f8c/verible-v0.0-3756-gda9a0f8c-{get_os()}"
    verible_dir = "verible"
    verible_binary = get_exe()
    expected_md5 = "450bc9e482aa124157647a64bb50404b"

    def md5sum(filepath):
        hash_md5 = hashlib.md5()
        with open(filepath, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
        return hash_md5.hexdigest()

    if os.path.isfile(verible_binary):
        if md5sum(verible_binary) == expected_md5:
            return
        else:
            try:
                shutil.rmtree(verible_dir)
            except:
                pass
    with urllib.request.urlopen(url) as response:
        if response.status != 200:
            raise Exception("failed to download verible")
        tar_data = io.BytesIO(response.read())

    with tempfile.TemporaryDirectory() as tmpdirname:
        if ".zip" in get_os():
            with zipfile.ZipFile(tar_data) as zip_ref:
                zip_ref.extractall(tmpdirname)
        else:
            with tarfile.open(fileobj=tar_data, mode="r:gz") as tar:
                try:
                    #3.13+
                    tar.extractall(path=tmpdirname,filter="data")
                except TypeError:
                    #3.12-
                    tar.extractall(path=tmpdirname)
        extracted_contents = os.listdir(tmpdirname)
        if len(extracted_contents) != 1:
            raise Exception("Unexpected contents in tar file.")

        extracted_path = os.path.join(tmpdirname, extracted_contents[0])
        try:
            shutil.copytree(extracted_path, verible_dir)
        except FileExistsError:
            pass
        shutil.rmtree(extracted_path)



def main():
    get_verible_format()
    args_to_forward = sys.argv[1:]

    result = subprocess.run([get_exe()] + sys.argv[1:])

    # Optional: exit with the same return code
    sys.exit(result.returncode)

if __name__ == "__main__":
    main()
