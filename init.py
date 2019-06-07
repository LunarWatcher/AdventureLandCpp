import os

packages = []
try:
    import scons
except:
    print("Installing SCons...")
    packages.append("scons")

try: 
    import conans
except:
    print("Installing Conan...")
    packages.append("conan")

if (len(packages) != 0):
    from pip._internal import main
    pipArgs = ["install"] + packages
    main(pipArgs)

os.system("git submodule update --init --recursive")

if not os.path.exists("build"):
    os.mkdir("build")

os.chdir("build")
result = os.system("conan install ../ --build missing")

if (result != 0):
    raise Exception("Failed to install conan packages.");
os.chdir("..");

# Symlinks
import VSConan.init
