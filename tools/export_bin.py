import os
import shutil

Import("env")


def export_bin(source, target, env):
    src = str(target[0])
    project_dir = env.subst("$PROJECT_DIR")
    destinations = [
        os.path.join(project_dir, "build", "firmware.bin"),
        os.path.join(project_dir, "tools", "mac", "firmwareUpdater", "firmware.bin"),
        os.path.join(project_dir, "tools", "win", "firmwareUpdater", "firmware.bin"),
    ]

    for dst in destinations:
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(src, dst)
        print("Exported BIN: %s" % dst)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", export_bin)
