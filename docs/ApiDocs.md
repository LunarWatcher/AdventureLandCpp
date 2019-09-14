The base path used in this file is `https://adventure.land`, unless otherwise stated. This is only endpoint documentation. If something doesn't make sense, the wrapper guide might be more useful. 

# Special POST

These require special handling. As a general rule, these are POST. However, they require a `method` in the POST form data matching the endpoint. As an example, the endpoint `servers_and_characters` requires a `method` field in the post data set to `servers_and_characters`. 

* `/api/signup_or_login` - the login endpoint. Expects email and password, as well as `"only_login": true` in a map called `arguments` passed as formData.
* `/api/servers_and_characters` - retrieves the characters for the associated account, as well as servers. Requires login. 
* `/api/get_servers` - retrieves the servers

# GET
* `/data.js` - returns a JS variable containing the game data. 
