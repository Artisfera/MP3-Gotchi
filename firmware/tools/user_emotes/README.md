# MP3-Gotchi User Emotes

Create one folder per emote here.

Example:

```text
firmware/user_emotes/happy/
  01-eyes.png
  02-mouth.png
  03-nose.png
```

PNG files are host-side source layers only. They must be full-screen `96x64` images, just like the source sprite exports.
Black pixels and transparent pixels are treated as empty space.

Run:

```powershell
py firmware\tools\user_emotes\build_user_emotes.py
```

The tool creates `emote.png` previews for humans and writes generated sparse RGB565 data to the latest sketch `UserEmotes.h`.
