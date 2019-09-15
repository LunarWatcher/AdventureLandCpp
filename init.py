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


init = input("Init submodules? (True/False): ") in ["yes", "1", "true", "True"]
if (init):
    print("Initializing submodules...")
    os.system("git submodule update --init --recursive")

build = input("Non-standard build location? (Enter to skip, folder name (i.e. my-build) otherwise)")
if (build == ""):
    build = "build"
if not os.path.exists(build):
    os.mkdir(build)

os.chdir(build)
result = os.system("conan install ../ --build missing")

if (result != 0):
    raise Exception("Failed to install conan packages.");
os.chdir("..");


init = input("Create symlinks? (True/False): ") in ["yes", "1", "true", "True"]

if (init):
    # Symlinks
    from VSConan.init import run
    run(buildFolder = build);    
