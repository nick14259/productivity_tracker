Import("env")
from datetime import datetime

def create_version_file(target, source, env):
    version = datetime.now().strftime("%Y%m%d_%H%M%S")
    with open("include/version.h", "w") as f:
        f.write(f'#define FIRMWARE_VERSION "{version}"\n')

env.AddPreAction("buildprog", create_version_file)