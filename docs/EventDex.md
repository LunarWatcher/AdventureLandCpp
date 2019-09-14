# EventDex 
**NOTE:** Work in progress. This list evolves as I find new events. 

Core events are the events you **need** to have a working wrapper. Without these, your code will not work as expected. The other events are also necessary, but they aren't necessarily critical for your code to work. Some of these events have also meen mentioned in the wrapper guide.

## Events sent by the server

### Core events

* `welcome` - sent by the server when you intially connect. 
* `entities` - sends updates about other entities. Not 100% real-time (see the list of pitfalls for more info).
    The event comes with a JSON object with two arrays: monsters and players. To match the browser version, you need to manually set the type of a player to character when iterating. Same for monsters, but slightly more confusing - set the monster's mtype to the type defined in the response, and the type to monster. 
    Some pseudo-code to clarify potential confusion:

    ```python
    for player in event["players"]:
        player["type"] = "character"
        # process player
    for monster in event["monsters"]:
        monster["mtype"] = monster["type"]
        monster["type"] = "monster"
        # Process mob
    ```
    Also note that there's some sanitation that needs to be done - see the list of pitfalls.
    For the details of implementing this, see BaseImplementations.md
* `player` - contains updates to the player. This, like the `start` event, has data equivalent to the player's JSON (which means you can just update your existing JSON with the received). Emphasis on update - don't replace. If you use the raw JSON to store data, this is critical.
* `new_map` - received whenever the map changes for whatever reason
* `disappear`/`notthere` - Triggered when a mob or player disappears. It may contain a key for what happened (`teleport` or `invis`). Either way, your response should be to set the entity as dead. You'll get an update if that changes. 
* `death` - this is the third option for disappearance, but this is a critical function. Here, you change the state of the entity in the entities map and set `dead` to `true`. This, like several other functions, are split up instead of kept in a core system. 

### Other events

* `cm` - called when a CM is received. Contains JSON data that may be a string, number, object, array, or any other form of valid data seen from the perspective of `socket.emit()`. 
* `invite` - received when a party invite is received. `event["name"]` tells you who sent it.
* `request` - received when you get a party request. Also contains `event["name"]`, like the `invite` event
* `party_update` - received when the party changes. `event["party"]` contains various standard information about the characters in the party.

## Events sent by the client 

### Core events

* `loaded` - sent in response to the `welcome` event. TODO: include sample data (readers: see WrapperGuide.md in the meanwhile)
* `auth` - sent when the first `entities` event with `event["type"] == "all"` is received. TODO: Include sample data (readers: see WrapperGuide.md in the meanwhile)

### Other events

TODO: Include movement and other standard calls 
