# Status notice

Note that this is a work in progress, and only supports primitive bot scripting at the moment. If you're looking for a fully working alternative while this is being built, check out [ALBot](https://github.com/NexusNull/ALBot/), or stick to the website/steam client. 

If you want to get started with this client, the rough equivalent of default.js is available in the `apidocs` folder. Note that the code is subject to unnotified changes while the library is being developed.

# Library developer setup

Run init.py. This will set up the dependencies. If you decide to use virtualization (Docker), it's still recommended you do this before installing it in Docker. This is because of autocomplete. If you don't use autocomplete, you can naturally skip that. 

This is intended to be a library, but in its current state, it isn't importable as one. Building can be done by running SCons.



