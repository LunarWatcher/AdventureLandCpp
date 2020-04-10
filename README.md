# Project shutdown notice 

For the past few months, the issue I have over and over run into has been pathfinding. Pathfinding as it stands is slow and inefficient. In January, I bumped [the pathfinding issue](https://github.com/LunarWatcher/AdventureLandCpp/issues/4) to a high priority. I've since spent a lot of time trying to make it work.

I have made several attempts to fix it. One attempt used a different algorithm, and that made it worse. With recent map changes, there's places where it thinks it has a path, but completely breaks when it tries to execute that path. This approach used a brute force approach, where every 7 pixels relative to some position is a "node". On a map that's thousands of pixels big, you can already guess this is inefficient. 

My last attempt tried to use triangulation. This was easier said than done. After an initial failure to attempt to find a library, I decided to write some code from scratch. There's surprisingly little information on this online, and me having no idea how to go about it presented several challenges. I managed to write some basic stuff, but it wasn't even constrained. Attempts to add constraints and account for padding failed. How adding holes to this outer hull is done is still something I have no idea how t odi. I did after this, however, find a library. Specifically, I found [earcut.hpp](https://github.com/mapbox/earcut.hpp). This looked promising, so I started to implement it. Once again... challenging. 

The library does its job, don't get me wrong. it just isn't meant for this type of use, where there's complex shapes. In addition, there's lines where the outer hull splits into several lines, and then merge back together. The library isn't meant for this. Of course, it can be done. Other players have as well. I just have no idea how, and in spite of having spent hours on research, I still have no idea how to generate a navmesh. With C++, it's mostly Recast, Unity, or Unreal. None of those three can easily be applied. 

The only remaining option I had to make this work was triangulation in some form. Proper pathfinding is a big deal, because it's the only way the library can be used to truly automate the game. 

Therefore, this project will no longer be developed. Pathfinding is such a blocking issue that I can't just ignore it and move on with other parts of the library. The game is constantly evolving, as the home page says. I can't keep up with building the library, keeping it up to date, and dealing with pathfinding. I'm willing to bet I'm missing something super obvious, but I can't find any usable resources on how this is done. My own unsourced attempts didn't yield anything either. 

I might find a solution some day, or even learn about it, but for now, I'm going to move on. I do have some alternate solutions to work around these issues, but they all require manual game data modifications. Keeping up with that is going to be extremely hard. So for now, I see no other choice but to leave it. 

That being said, I'm going to leave the repo open (not archived) just on the off chance someone who actually knows how to generate navmeshes finds this and takes the time to submit a pull request. Until that happens, or like I said, I figure it out, the project will be on ice. 

# Status notice

Note that this is a work in progress, and only supports primitive bot scripting at the moment. If you're looking for a fully working alternative while this is being built, check out [ALBot](https://github.com/NexusNull/ALBot/), or stick to the website/steam client. 

If you want to get started with this client, the rough equivalent of default.js is available in the `apidocs` folder. Note that the code is subject to unnotified changes while the library is being developed.

# Library developer setup

Run init.py. This will set up the dependencies. If you decide to use virtualization (Docker), it's still recommended you do this before installing it in Docker. This is because of autocomplete. If you don't use autocomplete, you can naturally skip that. 

This is intended to be a library, but in its current state, it isn't importable as one. Building can be done by running SCons.



