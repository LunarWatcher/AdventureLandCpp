from SConsStandard import EnvMod
import os 

env = EnvMod.getEnvironment()
env.withConan()
env.environment['ENV']['ASAN_OPTIONS'] = 'halt_on_error=0;detect_leaks=0'
lib = env.SConscript("src/SConscript", variant_dir="src", duplicate = 0)


env.withLibraries("AdvLandCpp", append = False)
env.appendLibPath("#" + env.getBinPath())

env.appendSourcePath("#src/")

program = env.SConscript("DummyApp/SConscript", variant_dir="DummyApp", 
                         exports = { "lib": lib }, duplicate = 0)

Depends(program, lib)
