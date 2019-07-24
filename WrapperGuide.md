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
