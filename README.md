# Status notice

Note that this is a work in progress, and only supports primitive bot scripting at the moment. If you're looking for a fully working alternative while this is being built, check out [ALBot](https://github.com/NexusNull/ALBot/), or stick to the website/steam client. 

If you want to get started with this client, the rough equivalent of default.js is available in the `apidocs` folder. Note that the code is subject to unnotified changes while the library is being developed.

# Setup

If you haven't already, make sure your conan profile is set to use C++ 11:

```
conan profile update settings.compiler.libcxx=libstdc++11 Profile
```

Visual Studio handles this differently - keep that in mind if you get undefined references. This project is, however, is written with Clang, and is untested with VS.

This is a required step with Clang and possibly GCC to link some dependencies properly. If this is missing, these libraries cause issues with undefined references at compile time. The project itself builds around C++ 17.

Additionally, a library is used that isn't in conan-center. Therefore, the remote has to be added manually:

```
conan remote add <name> https://api.bintray.com/conan/degoodmanwilson/opensource
```

# Library developer setup

Run init.py. This will set up the dependencies. If you decide to use virtualization (Docker), it's still recommended you do this before installing it in Docker. This is because of autocomplete. If you don't use autocomplete, you can naturally skip that. 

This is intended to be a library, but in its current state, it isn't importable as one. Building can be done by running SCons.

# Library use setup

This project builds around SCons. Other build systems either need to invoke SCons and manually find the paths, or in some other way compile it. A conan package is planned in the future. Note that export as an SConstruct isn't fully supported yet, and like everything else, is a work in progress.

If you're on Windows, you'll need to install the Win32 Python package:

```
pip install pypiwin32
```
This is to enable support for the longer compile lines, should they exceed the limits Windows imposes at 1k chars.

# Known problems 

## Undefined references to LibUV

I'm not entierly sure why this is triggered, but I imagine it's related to the central build process. If you find yourself with undefined references to libuv when compiling, build it locally:

```
$ cd build-folder-name
$ conan install libuv/1.31.0@bincrafters/stable --build
```

