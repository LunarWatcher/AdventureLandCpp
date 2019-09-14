# List of common implementation pitfalls

## Event values for the `entities` event 

If your language, like mine, is strict on types, this is a pitfall. 

AL has in the past mixed values for the `rip` key of entities (some have true/false, others have 1/0). For strict languages (where 1/0 isn't accepted as a bool, or the JSON library explicitly requires true/false to return a bool), this will cause problems.

It's pretty easy to fix. Add a function called with each event:

```cpp
void preprocessEntity(mutable json entity) {
    if entity.hasKey("rip") and entity["rip"].isNumber():
        entity["rip"] = (entity["rip"].getAsInt() == 1); 
}
```

## Entities not moving 

This is a very, very large pitfall. Believe it or not, the movement calculations are done locally on a separate thread. If you don't process movement, nothing moves. You can also get movement through events, but these aren't frequent enough to possibly match local processing. 

## Map not updating when transporting/magiporting/using doors

Did you miss the `new_map` event? 



