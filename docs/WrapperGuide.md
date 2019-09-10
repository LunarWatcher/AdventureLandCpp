# General adventure.land wrapper advice

There isn't much details on creating custom wrappers aside reverse-engineering other wrappers. This file aims to make this information publicly available in plain-text, in order to facilitate easier development for wrappers in other languages, or just improvements off existing ones for that matter. 

Note that this doesn't cover finding the right tech stack - you'll need to do that on your own. That being said, you need one or more libraries that're capable of handling HTTP requests and websockets. **This is the minimal you need for the requests themselves**. Logging, the client-sided API, all of that is up to you. 

## Disclaimer

This is far from complete at the moment, but it'll expand as I start coming to different aspects of the API. That being said, pull requests improving/expanding this document are appreciated. 

## Login 

Login is the first part, and it's relatively easy. You need to be able to GET and POST (POST needs to be able to handle form arguments, see the resulting request below). 

The first thing I do is connect to the main page. This is to check if AL is up, and/or making sure  I can connect. This part is relatively easy: `GET` `https://adventure.land` and make sure you can connect. 

I check for the 200 status code here.

If that succeeds, proceed with the login. `POST` to `https://adventure.land/api/signup_or_login`. This is the endpoint that handles login. 

There are three pitfalls here. First of all, as a part of the form data, you'll need to add `"method": "signup_or_login"`. Secondly, the email and password are bundled in an `arguments` key (see the structure). Finally, along with the email and password in the `arguments` key, you'll need `"only_login": "true"`. 

Taking in generic terms, this is what the resulting JSON should look like:

```json
{
    "formData": {
        "method": "signup_or_login",
        "arguments": { 
            "email": "your email", 
            "password": "your password",
            "only_login": true
        }
    }
}
```

If your library doesn't support complex constructs passed to a given formData key, you could do like I did -- hard-code the map:

```cpp
form.set("arguments", "{\"email\":\"" + email + "\", \"password\": \"" + password + "\", \"only_login\": true}");
```

The point being, achieve the JSON structure shown earlier. Otherwise, AL will reject the login attempt. If you're missing the `arguments` part, you'll get a message like this:

```json
[{"message": "Invalid Email", "type": "ui_error"}]
```

Which brings me to my next part: error handling. If you get a JSON response where the type is `ui_error`, login has failed. 

Otherwise, you'll get a response like this:

```json
[{"message": "Logged In!", "type": "message"}, {"html": "<lots of HTML omitted>", "type": "content"}]
```

You don't need anything here aside verification. The HTML doesn't provide you with any data you need, so all you need to do is check if you get a `ui_error` or not. If you do, login has failed. Otherwise, you're good.

There's one final step before you can move on: you need to get the session cookie. This is represented by a string value, and there's several ways to get it. One is using the `set-cookie` header with some regex, another is letting your library parse the cookies for you, and get the auth cookie (meaning a cookie with the key/name `auth`). 

Now, if you split this by `-`, the first value will be your user ID. The full cookie value is your session cookie, which you'll need mostly for HTTP requests.

Unfortunately, AL makes this... complicated. To connect, you also need a user auth token, which is hidden somewhere in the HTML of the index page when you're logged in. You **need** this token to continue with websocket connection.

Basically, `GET` the front page, and add the auth cookie with the session cookie (which you got from the auth cookie) to the request. This also lets you validate your session, but getting the user_auth key is critical to continue. 

When you've sent the request, you need to parse the HTML, [and what better way to do that than with regex](https://stackoverflow.com/a/1732454/6296561)?

Run this regex on the HTML:

```regex
user_auth=\"([a-zA-Z0-9]+)\"
```

The quotes are escaped here, but this is made for the variant of regex supported by C++ (and potentially other similar languages), but you might need to modify it for other languages. If your language has raw strings, you probably don't need to escape it at all.

## Getting game data

Parsing the game data can be done in several ways, but I'll be using string replacement and JSON parsing. 

The game data is formatted as a JavaScript variable, which of course can't be added directly to most languages. One option is evaluating the JS and somehow getting the data from there, but I prefer removing the JS parts to get parseable JSON. I personally use substring to remove the first 6 chars (`var G=`) and the last char (which is a semicolon). 

You can GET the raw data from `https://adventure.land/data.js`. If you open it up, you'll also see the reason you can't parse it directly, as well as the reason why it's called `G` in-game. This also contains the world itself, which is why you need it. 

## Getting characters and servers

Endpoint: `https://adventure.land/api/servers_and_characters`. This requires the auth cookie as well, so remember to add the auth cookie if you don't have a session that automatically store these values, and add them with the request. 

Here, similarly to logging in, you need to pass a `method` in the form data, which in this case is  `servers_and_characters`. If you're not logged in (which means the cookie didn't join the request if you did proper validation during the login phase), you'll get a 200 status code along with some JSON:

```json
[{"args": ["Not logged in."], "type": "func", "func": "add_log"}]
```

But if the cookie is passed and everything else is fine, you'll get a response similar to this:

```json
[
  {
    "type": "servers_and_characters",
    "tutorial": {
      "progress": 100,
      "step": 0,
      "task": false,
      "completed": []
    },
    "characters": [
      {
        "name": "ZoetheWolf",
        "level": 68,
        "gender": "female",
        "online": 0,
        "type": "ranger",
        "id": "<id>"
      },
      {
        "name": "Amethyst",
        "level": 66,
        "gender": "female",
        "online": 0,
        "type": "mage",
        "id": "<id>"
      }
      <more in the same format>
    ],
    "servers": [
      {
        "players": 0,
        "region": "ASIA",
        "name": "I"
      },
      {
        "players": 16,
        "region": "EU",
        "name": "I"
      },
      {
        "players": 47,
        "region": "EU",
        "name": "II"
      },
      {
        "players": 3,
        "region": "EU",
        "name": "PVP"
      },
      {
        "players": 37,
        "region": "US",
        "name": "I"
      },
      {
        "players": 23,
        "region": "US",
        "name": "II"
      },
      {
        "players": 9,
        "region": "US",
        "name": "PVP"
      }
    ]
  }
]
```

You can parse both the servers and characters by the current account from this. If you need runtime validation/re-checking servers, you can use the `get_servers` endpoint, as outlined in the ApiDocs file.

# Connecting

I highly suggest using a Socket.IO client. Using the approach I did works, but it's horribly inefficient to create everything from scratch if you have an option to do something else. Python and JS both have decent and up-to-date clients. C++ doesn't, which is why I used some hacks. That's documented elsewhere.

## Connecting with Socket.IO

TODO

## Connecting with websockets

You'll need to connect to `wss://address:port/socket.io/?EIO=3&transport=websocket`, where address:port is something you should've retrieved earlier. 

Parsing the websocket responses is an entire topic on its own, because Socket.IO is a protocol as well. It sends messages in a format that the socket.IO client usually parses. When you're using websockets, you get, and need to parse, the raw data. See also ParsingSocketIO.md for more info on a minimal way of parsing.

## When you've connected

This is where it gets easy again. When you've connected and linked your code properly, you'll need to wait for the welcome event. When you get this, you emit a "loaded" event in return. I just hard-coded some values:

```
socket.emit("loaded", {"success": 1, "width": 1920, "height": 1080, "scale": 2});
```

(the game uses scale = 2 by default. Using other values probably won't affect gameplay). 

After this, you need to listen for the entity event. It's at this point you can log in your actual character. You'll need the auth token from earlier, along with the player ID and some other values. 

Basically, the connection and initialization is a flow of events:

```yaml
Server: welcome
Client: loaded
Server: entities 
Client: auth
Server: start
```

This assumes every step succeeds of course. Unfortunately, error responses aren't that useful, mainly because there's minimal of them.

Anyway, after the loaded event, you should get an entities event. This is a very active event, but you're looking for the first event (can be checked with a bool switch - just remember to toggle it if you disconnect) where the type is all (`data["type"] == "all"`). To this, you respond with an auth event:

```cpp
emit("auth", {
    "user": userId, // the first part of the auth cookie.
    "character": characterId, // the character of the ID to connect. DO NOT confuse this with the userId.
    "auth": authToken, // This is the auth token 
    "width": 1920,
    "height": 1080,
    "scale": 2, 
    "no_html": true,
    "no_graphics": true
    
});
```

If everything is correct, you'll now receive the start event. This contains JSON data, which contains data for the character you need to get started. 

## Congratulations - you've successfully connected!

At this point, the next step is to get started with the API. Unfortunately, this is where reverse-engineering starts. I can't cover every single method in a language-neutral way without losing any important info. That being said, these methods are, in my experience, the worst to implement:

* `canMove()`
* `smartMove()`
* Processing of movement 

## The next steps 

Aside the code, take a look at EventDex.md and Pitfalls.md. EventDex.md contains a list of the events, as well as a mini crash-course on how to use it. There's also several central events necessary for basic function (which I had to learn the hard way). Pitfalls.md contains various problems that aren't easy to detect, but that're very real. 

## Problems? Open an issue! 

If you have a general problem with connecting or getting started, feel free to open an issue. For more complex, implementation-specific problems (such as problems implementing a specific method, etc), issues aren't the best platform. Therefore, using Discord (either the [official AL server](https://discord.gg/gMHpqZM) or [my personal server (not centered around Adventure.Land)](https://discord.gg/bjZD42j)) is the better option.<sub>At least for now - this might change later.</sub>

Because programming languages vary, I can't guarantee I'll be able to help with these types of issues, but I'll try. 

**Note:** These docs are written while I work on the library, and may contain errors. Because the game is in alpha and rapidly evolves, these docs may also become outdated without me knowing or noticing. 

That being said, if you find something wrong in the docs, and you know what's wrong, feel free to open a pull request fixing the problem, and I'll review it. 

