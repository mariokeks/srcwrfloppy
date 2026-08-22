# 💾 SRCWRFloppy

Asynchronous replay file saving **and loading** for [shavit's bhoptimer](https://github.com/shavitush/bhoptimer).

This is a fork of [srcwr/srcwrfloppy](https://github.com/srcwr/srcwrfloppy), a SourceMod extension that writes replay files on a background thread instead of blocking the game thread. The original `SRCWRFloppy_AsyncSaveReplay` native to write replays is unmodified in this fork and works just as in the original extension.

## What this fork adds

- **`SRCWRFloppy_AsyncLoadReplayFrames`** - asynchronously loads replay frames from a file on a background thread, without blocking the game thread.
- **`SRCWRFloppy_LoadReplayCache`** - a stock to simplify `SRCWRFloppy_AsyncLoadReplayFrames` and replace the original `LoadReplayCache` that supports asynchronous & synchronous loading depending on the replay or extension itself. It reads the replay header synchronously and then loads all frames asynchronously (falls back to synchrounous `LoadReplayCache` for older replays or if forced).
- **`SRCWRFloppy_AsyncSaveReplayEx`** - and some optional parameters in `SRCWRFloppy_AsyncLoadReplayFrames` are custom made for our replay version to include additional replay data and can be ignored.

>**Note:** the changes to the Rust extension in this fork are AI generated. The SourcePawn include and the public API are hand-reviewed.

## Installing

1. Download the [latest release](https://github.com/mariokeks/srcwrfloppy/releases) zip.
2. Extract it directly into your server's game directory.
3. Restart your server, or run `sm exts load srcwr💾` from the server console.

## Usage

The full documented API can be found [here](https://github.com/mariokeks/srcwrfloppy/blob/master/srcwrtimer/addons/sourcemod/scripting/include/srcwr/floppy.inc) and an example as complete test command can be found [here](https://github.com/mariokeks/srcwrfloppy/blob/master/srcwrtimer/addons/sourcemod/scripting/test_command.sp).

## Support

If you find a bug or have a suggestion, feel free to [open an issue](srcwrtimer/addons/sourcemod/scripting/test_command.sp).
