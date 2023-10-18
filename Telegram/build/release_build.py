#!/usr/bin/python

# usage release_build.py <target>
import sys
import os
import subprocess

_THIS_DIR = os.path.dirname(__file__)

TARGET_MAP = {
    "Win32": "win",
    "x64": "win64"
}

def main():
    if len(sys.argv) != 2:
        print("Usage release_build.py <target>")
        sys.exit(1)
    target = sys.argv[1]
    target = TARGET_MAP.get(target, target)
    api_id = os.environ["API_ID"]
    api_hash = os.environ["API_HASH"]

    file_target = os.path.join(_THIS_DIR, "target")
    with open(file_target, "w") as f:
        f.write(target)

    # Prepare dropbox path
    dropbox_path = os.path.join(_THIS_DIR, "../../../Dropbox/Telegram/symbols")
    if not os.path.exists(dropbox_path):
        os.makedirs(dropbox_path)
    # Prepare release path
    release_path = os.path.join(_THIS_DIR, "../../../Projects/backup/tdesktop")
    if not os.path.exists(release_path):
        os.makedirs(release_path)

    private_path = os.path.join(_THIS_DIR, "../../../DesktopPrivate")
    if not os.path.exists(private_path):
        os.makedirs(private_path)
    with open(os.path.join(private_path, "custom_api_id.h"), "w") as f:
        f.write("const long ApiId = %s\n" % (api_id))
        f.write("const char* ApiHash = \"%s\"\n" % (api_hash))

    # RSA Keys
    with open(os.path.join(private_path, "alpha_private.h"), "w") as f:
        f.write("static const char *AlphaPrivateKey = \"\";\n")
    if "RSA_PRIVATE" in os.environ:
        with open(os.path.join(private_path, "packer_private.h"), "w") as f:
            f.write(os.environ["RSA_PRIVATE"])
    with open(os.path.join(private_path, "Sign.bat"), "w") as f:
        pass

    cmd = "build.bat -DDESKTOP_APP_SPECIAL_TARGET=" + target

    os.system(cmd)

if __name__ == "__main__":
    main()