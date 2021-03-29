# Death Counter mod for Mass Effect (2007)

Counts player's deaths and persists the number in a file (`Binaries\deathcount.txt`). The file is opened and closed every read/write, so other programs should be able to read it in the meantime. If the file is locked by some other process, naturally, the mod won't be able to use it.

This mod provides two console commands:

  - `dc.togglehud` - either start or stop displaying the death count in the top right corner of the screen (visible only in gametime, Scaleform UI hides it);
  - `dc.resetcount` - reset the death count and update the file accordingly.

To install, put `ModDeathCount.asi` or `ModDeathCount.dll` into `Binaries\asi\` and enable the binkw bypass, which will automatically inject this mod on startup. Alternatively, use a DLL injector of your preference.

The mod should [just work](https://youtu.be/YPN0qhSyWy8) with the english Origin version of the game (1.2.20608.0).

*AV software, including Windows Defender, may dislike this mod because it detours several parts of the game's executable, which is a basic malware technique. The mod, however, is not malware, and if you are even half as paranoid as you should be, feel free to grab Visual Studio and compile the mod from source.*