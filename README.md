# TheHunter_Classic_Linux-contextFix
This repo designed to fix the problem with context menu popping when playing thehunter classic and using right click.

# theHunter Classic Proton/Wine CEF Context Menu Fix

The bug shows a Chromium/CEF-style menu such as:

```text
Back
Forward
Print
View Source

We inject a custom WINHTTP.dll into the game using Wine's DLL override mechanism.

This DLL:

hooks Windows menu APIs (TrackPopupMenu, etc.)
blocks the unwanted context menu without no performance impact.


OPTION 1 Use prebuilt:

download WINHTTP.dll
and place it within the game files, one in near launcher.exe and one in the game directory

like so:
cp WINHTTP.dll "/path/to/SteamLibrary/steamapps/common/theHunter/launcher/"
cp WINHTTP.dll "/path/to/SteamLibrary/steamapps/common/theHunter/game/"

In steam launch options:
WINEDEBUG=+loaddll  WINEDLLOVERRIDES="winhttp=n,b" %command%

(Personally those are mine: vblank_mode=0 PROTON_LOG=1  WINEDEBUG=+loaddll  WINEDLLOVERRIDES="winhttp=n,b" %command%)

you are done at this point. you can check out logs at the prefix drive_c location and enable proton logging and a log file will be available at your homedir (steam-253710.log)

Now, if you are concerned about this prebuilt file and want to inspec and compile it yourself - here is how to do it.

OPTION 2 - Build yourself:

Requirements
Linux
MinGW-w64 (32-bit)
git

Example here is Arch.
1) install deps: sudo pacman -S mingw-w64-gcc git
2) clone minhook: git clone https://github.com/TsudaKageyu/minhook.git

3) prepare the project dirs:

The project tree should look like this:

project/
├── winhttp_proxy.c
└── minhook/

4) Compile

cd to project root and run the following:

i686-w64-mingw32-gcc \
  -shared winhttp_proxy.c \
  minhook/src/hook.c \
  minhook/src/trampoline.c \
  minhook/src/buffer.c \
  minhook/src/hde/hde32.c \
  -Iminhook/include \
  -Iminhook/src/hde \
  -o WINHTTP.dll \
  -luser32 \
  -Wl,--add-stdcall-alias

You're expected to get a PE32 executable called WINHTTP.dll

then you can install it intro the needed directories

cp WINHTTP.dll "/path/to/SteamLibrary/steamapps/common/theHunter/launcher/"
cp WINHTTP.dll "/path/to/SteamLibrary/steamapps/common/theHunter/game/"

In steam launch options:
WINEDEBUG=+loaddll  WINEDLLOVERRIDES="winhttp=n,b" %command%

(Personally those are mine: vblank_mode=0 PROTON_LOG=1  WINEDEBUG=+loaddll  WINEDLLOVERRIDES="winhttp=n,b" %command%)



FOR BOTH:

logging is at the following places:
steam log - grep -i winhttp ~/steam-253710.log
dll hook log - /path/to/SteamLibrary/steamapps/compatdata/253710/pfx/drive_c/hunter_winhttp_proxy.log

expected output at logs:
Proxy loaded
Hooks installed
Blocked TrackPopupMenuEx

Credits
MinHook: https://github.com/TsudaKageyu/minhook
Wine / Proton

This fix uses a proper runtime hook + API proxy, not a workaround.

Reusable for:

other CEF bugs
Wine UI glitches
DLL injection debugging

If it helped — ⭐ the repo
