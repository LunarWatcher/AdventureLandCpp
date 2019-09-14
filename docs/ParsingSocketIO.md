# Parsing the Socket.IO standard

If you've connected to the server with Socket.IO, you've chosen the hard option. Socket.IO sends messages in its own format, which you'll need to parse if you want to interact with the game. 

Note that due to a lack of data, this doesn't cover the types for ack, binary events, or binary ack - they're probably not required by the game either. See [issue #1](/LunarWatcher/AdventureLandCpp/issues/1). 

The first thing you need to do is understand the system. You'll receive messages such as:

```json
0{"sid":"<redacted>","upgrades":[],"pingInterval":4000,"pingTimeout":12000}
42["welcome",{"map":"main","in":"main","x":-101,"y":1417,"region":"EU","name":"I","pvp":false,"gameplay":"normal","info":{}}]
```

To understand what this means, a [C++ implementation of the protocol](https://github.com/socketio/socket.io-client-cpp/blob/6063cb1d612f6ca0232d4134a018053fb8faea20/src/internal/sio_packet.h#L20) demonstrates this pretty well. In that class, you can see two enums called frame_type and type. If you're used to websockets, please don't confuse frame_type with websocket type. These are a separate system.

The frame is the first number. On the two examples, you can see the first one is connected, and the other is a message. You'll also need to parse the first one for the pingInterval. 

Secondly, we have the type. As far as I can tell, this is only used with the message frame type, and it's the second number in the welcome line. Connect is 0, so you may also get a websocket frame that simply contains `40`, or "message connect". I personally ignore this and use the 0 event to dispatch the `connect` event, but that's what it means. `41` is a message you've been disconnected, but it should be accompanied with a message of frame type `1`.

With an event (`42`), you'll get something like this:

```
["eventName", eventData]
```

`eventData` is pretty special, because it can be in any form acceptable by JSON, as far as I can tell. Which means you can get an array, you can get a map, you can get a string, but you could theoretically get nothing. As far as I know, `42` without at least the event name in an array shouldn't happen, so you can strip the numbers and parse the rest as JSON.

TL;DR: The basics of a Socket.IO message:

```
frameType[type][data]
```

Data can also be a single string in some cases.

**Common pitfall:** while 4 as the frame type is a message, the websocket type for all of these are messages. The websocket connection events are separate, and all the socket.IO strings are sent as websocket messages, which can be retrieved with `socket.recv()` in some implementations. 

# Pinging 

Pinging needs to be done at the pingInterval you received in the connection event. Fortunately, pinging is really easy; all you need to do is send a message containing `2`. If you remember the frame types, 2 is ping, and doesn't require any data. So all you need to ping is a time checker to make sure you don't spam ping the server (as of writing, pinging every 4000 milliseconds is enough), and:
```
webSocket.send("2");
```

# Copying socket.emit's behavior

If you're not familiar with what `socket.emit` does, it sends an event, and you now know the format of it! 

First of all, you're sending a message, which means you're using frame type 4. Because of events, the type used is 2. Additionally, we need an array with the event name, and data. Pretty easy now:

```
public void emit(string eventName, json data) {
    string message = "42[\"" + eventName + "\"," + data.toString() + "]";
    webSocket.send(message); 
}
```

`data.toString()` here converts the JSON object to a String. If you for some reason can't do that (no JSON library that supports converting objects to strings), I suggest just passing a string directly with pre-parsed JSON. Some languages, including C++, name the method `dump()`. Note that data should also allow string-only objects for full compliance.

# Copying socket.send's behavior

`socket.send` is not the same as a normal websocket message. In fact, it just [wraps socket.emit](https://stackoverflow.com/a/11500379/6296561) and sends a `message` event:

```
public void send(json data) {
    emit("message", data);
}
```

