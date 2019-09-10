# Base implementations of core concepts

This contains a short list (not intended to be exhaustive) of some implementations of core concepts. 

If there's code included in the examples, it's pseudocode, and not intended to be valid without modification. This is to avoid copy-pasta, because that leads to bad implementations. Try to understand what the code does - don't just copy it. 

I also aim to include the type where possible, to make it considerably easier to find.

## Entity processing 

After preprocessing the entities as mentioned in the `entities` event documentation (see EventDex.md), all the entities can be thrown into a single list called `entities`. Processing entities is pretty easy.

## Moving entities

First, you need to calculate the vectors. This is done using a standard formula (this is aimed to be the same as in the original JS implementation to avoid location correction).

```cpp
tuple calculateVectors (const json& entity) {
    double ref = sqrt(0.0001 + pow(entity["going_x"].getDouble() - entity["from_x"].getDouble(), 2) + pow(entity["going_y"].getDouble() - entity["from_y"].getDouble(), 2));
    double speed = entity["speed"];
    int vx = speed * (entity["going_x"].getDouble() - entity["from_x"].getDouble()) / ref;
    int vy = speed * (entity["going_y"].getDouble() - entity["from_x"].getDouble()) / ref;
    return [vx, vy];
}
```

After that, you'll need a thread to process the movement:

```python
lastTime = getCurrentTimeInMillis()
while running:
    now = getCurrentTimeInMillis()
    delta = now - lastTime
    lastTime = now;
    for player in bots:
        if not player.moving:
            continue;
        if (player.json["ref_speed"] != player.json["speed"]):
            set player.json ref_speed = player.json["speed"]
            set player.json from_x = player.json["x"]
            set player.json from_y = player.json["y"]
            assign (vx, vy = calculateVectors()) into player.json
            set player.json engagaged_move = entity["move_num"] 
        set player.x = ((double) player.x) + player.vx * min(delta, 50.0) / 1000.0
        set player.y = ((double) player.y) + player.vx * min(delta, 50.0) / 1000.0

        # In this if-statement, all the keys are equivalent to a key or an attribute of the player
        if (from_x <= going_x and x >= going_x - 0.1 or from_x >= going_x and x <= going_x + 0.1) or (from_y <= going_y and y >= going_y - 0.1 or from_y >= going_y and y <= going_y):
            set player x = going_x
            set player y = going_y
            set player moving = false

```

This needs to be repeated for the list of entities stored for each player as well. All of it is done like in the above code, but the rest is left as an exercise to the reader.
The remaining keys are used as a part of the movement system. from_x is the source x, and equivalent for y. going_x and going_y are the final destinations. This is covered in the move method implementation.



