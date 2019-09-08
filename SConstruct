import os 
import subprocess


envVars = Variables(None, ARGUMENTS)
envVars.Add(BoolVariable("mingw", "Whether to use MinGW or not. Windows only", False))
envVars.Add(BoolVariable("overrideMingw", "If, for some reason, MinGW mode breaks Clang or GCC on your system, enable this. Windows only", False))
envVars.Add(BoolVariable("debug", "Puts the program in debug mode. Adds debug information, removes -O3", False))

env = Environment()


def getSystemInfo():
    gcc = os.getenv("CXX") == "g++"
    clang = os.getenv("CXX") == "clang++"
    platform = env["PLATFORM"]
    if("win" in platform or "mingw" in platform or platform == "cygwin"):
        return ("windows", gcc, clang)
    # Pretty sure Mac isn't gonna be a problem. Aside that, there's Linux and UNIX derivatives that're the big ones
    # This list might need to be amended if there are other, non-standard operating systems out there. 
    return ("unix", gcc, clang)

def handleMingw(platform, gcc, clang):
    if (platform != "windows"):
        return;

    isMingw = env["mingw"] if "mingw" in env else False;
    if (not isMingw and (gcc or clang) and not ("overrideMingw" in env and env["overrideMingw"])):
        print("You haven't configured MinGW support, but you're running GCC or Clang. For compatibility with flags, " \
            "MinGW mode is forcibly toggled. If this breaks something for you, add \"overrideMingw=True\"")
        isMingw = True;
    if (isMingw): 
        # Because Windows sucks.
        # SCons forces argument style over to MSVS (which also uses a terrible argument style). 
        # Adding this fixes the arguments with Clang and GCC
        Tool("mingw")(env)


def installPackages():
    """
    This is intended as a fallback if init isn't run. This can technically trigger various issues with
    the build, but what can you do? This assumes the rest of the environment is in place. 
    """
    # Make the build directory if it doesn't exist yet
    if not os.path.exists("build"):
        os.makedirs("build")
    if (os.path.isfile("build/SConscript_conan")):
        return;
    print("WARN: Conan not initialized. Installing packages now. Note that " \
        "this ignores any missing packages, and may cause the build to fail.")
    # Then change the directory
    os.chdir("build")
    # Run conan
    result = os.system("conan install ../ --build missing")
    if(result != 0):
        raise Exception("Conan non-zero response. Failed.")
    # And finally, change back in case it's needed later.
    os.chdir("..")

def configureConan():
    # Build system setup.
    # Load it:
    # Note that this builds on Conan being executed in the build folder. 
    conan = SConscript("build/SConscript_conan")
    # Finally, merge the conan flags with the current environment. 
    # This is also why the function is called later
    env.MergeFlags(conan['conan'])
    # Expose this to make it returnable. 
    return conan 

(platform, gcc, clang) = getSystemInfo()
handleMingw(platform, gcc, clang)

if "TERM" in os.environ:
    # Terminal color hack
    env["ENV"]["TERM"] = os.environ["TERM"]
# Listen to the config:
env["CC"] = os.getenv("CC") or env["CC"]
env["CXX"] = os.getenv("CXX") or env["CXX"]
env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))
# Windows/unnatural compiler location hack
env["ENV"]["PATH"] = os.environ["PATH"]

# LLVM hack: Windows with Clang will attempt to use ar and will likely fail, because ar doesn't exist. 
# This might work if GCC is installed, but "install GCC" isn't a good solution.
# Doing this forces SCons to use llvm-ar in a way that works cross-platform. 
if(clang and platform is "windows"):
    env["AR"] = "llvm-ar"

if("LINK" in os.environ):
    env["LINK"] = os.environ["LINK"]

if (gcc or clang):
    compileFlags = ("-std=c++17 -pedantic -Wall -Wextra " + # I like warnings
        "-Wno-c++11-narrowing")                            # Except this one. Go away. Ktx
else:
    compileFlags = "/std:c++17"

if "debug" in env and env["debug"] is False:
    if (clang or gcc):
        compileFlags += " -O3 "
    else:
        compileFlags += " /O2 "
else:
    if (clang or gcc):
        compileFlags += " -g -fstandalone-debug -O0 "
    else:
        compileFlags += " /DEBUG " 
env.Append(CXXFLAGS=compileFlags)

hasDummyDir = os.path.exists("DummyApp")

cppPath = ["src/"]
libPath = ["bin/"]

env.Append(CPPPATH=cppPath)
env.Append(LIBPATH=libPath)


installPackages()
# Load the conan packages. This is done after setup to properly merge 
# conan with the current config. 
data = configureConan();

env.VariantDir("build/src", "src", duplicate=0)
stacklib = env.Library("bin/AdvLandCpp", Glob("build/src/lunarwatcher/*.cpp"))

env.Prepend(LIBS=["AdvLandCpp"])

if(hasDummyDir):
    env.VariantDir("build/testapp", "DummyApp", duplicate=0)
    env.Program("bin/DummyApp", "build/testapp/HelloWorld.cpp")
    env.Program("bin/Experimental", "build/testapp/CompilerTest.cpp")

