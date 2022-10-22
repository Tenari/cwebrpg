# A Web-based MMORPG that runs on a C++ webserver

A pthreads-based webserver written in C that holds the gamestate in memory (and makes backups to disk) with a js/html/css/png client bundle that displays the game state and sends user input to the server. The core idea is a long-running `/newupdates` path on the server that the js frontend continually pings, but the server just hangs in response until there is actually an update to respond with (or timeout saying nothing happened). All other actions are just typical webrequests that are responded to immediately.

The actual game is just my same coalition victory crap, minimal version that's fun.

## Compiling and Using the System

On a Mac use this command to compile the server:

`./build.sh`

To run the server type `./game [port] [threads]` into a terminal that is in the directory where the executable file is located.

then go to `http://localhost:[port]/` to view/play the game.

## Code conventions

- `CAPS` for constants
- `snake_case` for custom types
- `littleCamelCase` for functions
- `BigCamelCase` for variables
