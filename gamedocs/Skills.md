# Manually using skills 

Manually using skills is... tricky. Aside the basics, the various skills may have a different format, and different JSON. 

Whether you're writing around raw socket calls, or just want to specify who to target with your skills, these will come in handy. As for this library: While writing wrappers to automatically generate the data is an option, actually writing the functions for them ends up being awkward, especially for associative skills (such as cburst, which takes an array of arrays.). 

# The base skill call

```cpp
socket.emit("skill", { "name": "string name here" });
```

There's some exceptions, which I'll get back to later, that send an event with the name of the skill alone:

```cpp
socket.emit("skillName");
```

Some also take various arguments, but it's highly dependent on which skill.

One thing you'll need to know universally: The IDs are strings. You'll need this for some of the targeting skills. 

# Basic skills

Class: All

Like I said, some skills just send an event matching the skill name. These skills include:

* `respawn` 
* `town` (teleports the user into town)
* `stop` (stops movement and teleport)

# Special skills

## 3shot and 5shot

Class: Ranger

These are ID-only skills. They're easy in comparison to some of the others. 

Socket call:

```cpp
socket.emit("skill", {"name": "3shot", "ids": arrayOfIds});
```
(3shot naturally needs to be replaced with 5shot for 5shot)

Each mob has an ID, which is in the `id` JSON attribute for the entity. Same applies to players. 

## Blink

<sub>No, [not the Doctor Who episode](https://en.wikipedia.org/wiki/Blink_(Doctor_Who))</sub>

Socket call:

```cpp
socket.emit("skill", {"name": "blink", x: targetXCoords, y: targetYCoords})
```

This skill uses the name, like all the others using this call, but also pass the X and Y coordinates as event data

# Non-targeting skills 

Class: various (refer the game data for info on a specific skill)

These either don't target entities, or are used on the character.

The event structure:

```cpp
socket.emit("skill", {"name": "skillName"})
```

Affected skills:

* `invis` (Rogue)
* `partyheal` (Priest)
* `darkblessing`
* `agitate`
* `cleave` (Warrior)
* `stomp`
* `charge`
* `light` (Mage)
* `hardshell` (Warrior)
* `track` (Ranger)
* `warcry`
* `mcourage` (Merchant)
* `scare`

## Item-consuming

Some skills use items without targeting any entities. These skills use this format:

```cpp
socket.emit("skill", {"name": "skillName", num: indexInInventoryForItem})
```

You'll also manually have to find the slot for the item - this isn't something the server does.  

Affected skills:

* `pcoat` (Rogue)
    * Item: `poison` 
* `shadowstrike`/`phaseout`
    * Item: `shadowstone`

# Single target skills

This category includes skills that only target one entity. These may or may not consume items (depends on which skill - see the subcategories)

## No items 

Format: 
```cpp
socket.emit("skill", {"name": "skillName", "id": "string ID of the target"});
```

Affected sills:

* `supershot` (ranger)
* `quickpunch`
* `quickstab`
* `taunt`
* `curse`
* `burst` (Mage)
* `4fingers` (Ranger)
* `magiport`
* `absorb`
* `mluck` (Merchant)
* `rspeed` 
* `charm` (Linked to the item `charmer`)
* `mentalburst`
* `piercingshot` (Ranger)
* `huntersmark` (Ranger)

## Item consumption

The format is indentical to the non-targeting, item-consuming skills, but with an ID for the target:

```cpp
socket.emit("skill", {"name": "skillName", "num": inventorySlotIndexForItem, "id": targetEntityId})
```

There's some exceptions to these, and those are explicitly mentioned in the list. in order to avoid breaking it up too much, this category also includes skills that consume mana. Instead of `num`, the data has a JSON key called `mana`:

```json
{ "name": "skillName", "mana": "manaAmount", "id": "targetId"}
```

However, while testing, I can't seem to get it to work. This may be a bug and might be fixed later, or it's not supposed to work any more. If the latter is the case, the affected skill(s) will be moved to a different category. 

<sub>This list may differentiate between mana and item consumption if the list of skills ever grows large enough for it to matter, provided it isn't a bug</sub>

Affected skills:

* `revive` (Priest)
    * Item: `essenceoflife` 
* `poisonarrow` (Ranger)
    * Item: `poison`
* `throw` (Merchant)
    * Item: Any
* `energize` (Mage)
    * Uses mana. If no mana is specified, it's picked by the server. I'm not sure how the amount selection algorithm works.

