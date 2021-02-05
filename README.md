# Mass Effect (2007) native tools

A bunch of DLL plugins to experiment with the freshly-generated UE3 SDK for the game.

### ExampleConsoleCommands

Directly hooks the native Actor.ConsoleCommand, providing an example of arguably the neatest way to add new console commands in runtime.

Load it in, type "ping", see "PONG" in the opened cmd window.

### ExamplePostRender

Hijacks a UScript function - BioHUD.PostRender - with a native implementation which calls the ProcessInternal on the remaining stack and then draws some stuff on top the game's HUD.

An example of another way to detour an engine function without the blunt function pointer comparison in ProcessEvent or CallFunction - but doesn't always work. Getting input parameters / overriding the return value is an adventure, the length of which largely depends on the calling code.

### FunctionDumper

Utilizes the ol' reliable ProcessEvent hook to find all UFunction instances in runtime. Dumps a CSV list to `function_dump.txt`.

### LoggerCallFunction
### LoggerProcessEvent
### LoggerProcessInternal

Hooks UObject::CallFunction, UObject::ProcessEvent and UObject::ProcessInternal, respectively, and logs all calls to its target function. If you need to find or detour something, chances are you will find it in one of these plugins' logs: `log_callfunction.txt`, `log_processevent.txt`, `log_processinternal.txt`.
