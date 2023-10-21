#!/usr/bin/python

# usage release_build.py <target>
import sys
import os

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

    print("Set target = %s" % (target))
    file_target = os.path.join(_THIS_DIR, "target")
    with open(file_target, "w") as f:
        f.write(target)

    # Prepare dropbox path
    print("Stub output file locations")
    dropbox_path = os.path.join(_THIS_DIR, "../../../Dropbox/Telegram/symbols")
    if not os.path.exists(dropbox_path):
        os.makedirs(dropbox_path)
    # Prepare release path
    release_path = os.path.join(_THIS_DIR, "../../../Projects/backup/tdesktop")
    if not os.path.exists(release_path):
        os.makedirs(release_path)

    print("Generate DesktopPrivate")
    private_path = os.path.join(_THIS_DIR, "../../../DesktopPrivate")
    if not os.path.exists(private_path):
        os.makedirs(private_path)
    print("Generate custom_api_id.h")
    with open(os.path.join(private_path, "custom_api_id.h"), "w") as f:
        f.write("const long ApiId = %s\n" % (api_id))
        f.write("const char* ApiHash = \"%s\"\n" % (api_hash))

    # RSA Keys
    print("Generate alpha_private.h")
    with open(os.path.join(private_path, "alpha_private.h"), "w") as f:
        f.write("static const char *AlphaPrivateKey = \"\";\n")
    if "RSA_PRIVATE" in os.environ:
        print("Generate packer_private.h")
        rsa_key = os.environ["RSA_PRIVATE"]
        rsa_key = rsa_key.splitlines()
        RSA_STRING = "\\n\\\n".join(rsa_key)
        cpp_file = "const char *PrivateBetaKey = \"\\\n" +\
                   RSA_STRING +\
                   "\";\n\n" +\
                   "const char *PrivateKey = \"\\\n" +\
                   RSA_STRING +\
                   "\";\n"
        with open(os.path.join(private_path, "packer_private.h"), "w") as f:
            f.write(cpp_file)
    print("Generate Sign.bat")
    with open(os.path.join(private_path, "Sign.bat"), "w") as f:
        pass

    # Apply patches
    ## d3d
    cmd = "cd %s && git checkout ./validate_d3d_compiler.py" % (os.path.join(_THIS_DIR, "../../cmake"))
    print("Call %s" % (cmd))
    os.system(cmd)
    cmd = "cd %s && git apply %s" % (os.path.join(_THIS_DIR, "../../cmake"),
                                     os.path.join(_THIS_DIR, "patches/validate_d3d_compiler.patch"))
    print("Call %s" % (cmd))
    os.system(cmd)

    cmd = "build.bat -DDESKTOP_APP_SPECIAL_TARGET=" + target
    print("Call %s" % (cmd))
    sys.exit(os.system(cmd))

if __name__ == "__main__":
    main()
