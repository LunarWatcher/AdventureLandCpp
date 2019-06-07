# Status notice

Note that this is a work in progress, and currently doesn't work at all for actually writing a bot. 

# Setup

If you haven't already, make sure your conan profile is set to use C++ 11:

```
conan profile update settings.compiler.libcxx=libstdc++11
```

Visual Studio handles this differently - keep that in mind if you get undefined references. This project is, however, is written with Clang, and is untested with VS.


# Library developer setup

Run init.py. This will set up the dependencies. If you decide to use virtualization (Docker), it's still recommended you do this before installing it in Docker. This is because of autocomplete. If you don't use autocomplete, you can naturally skip that. 

This is intended to be a library, but in its current state, it isn't importable as one. Building can be done by running SCons.

# Library use setup

This project builds around SCons. Other build systems either need to invoke SCons and manually find the paths, or in some other way compile it. 


