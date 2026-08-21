// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 rtldg <rtldg@protonmail.com>

#if defined _srcwrfloppy_included
	#endinput
#endif
#define _srcwrfloppy_included

#include <shavit/replay-file>


/**
 * Called when an async replay save has completed.
 *
 * @param saved         True if the replay was saved successfully, false otherwise.
 * @param data          The value passed to SRCWRFloppy_AsyncSaveReplay[Ex].
 */
typedef ReplaySavedCallback = function void (bool saved, any data);

/**
 * Called when async replay frame loading has completed.
 *
 * @param loaded        True if frames were loaded successfully, false otherwise.
 * @param data          The value passed to SRCWRFloppy_AsyncLoadReplayFrames.
 */
typedef ReplayFramesLoadedCallback = function void (bool loaded, any data);

typeset LoadReplayCacheCallback
{
	/**
	 * Called when SRCWRFloppy_LoadReplayCache has completed.
	 *
	 * @param loaded        True if frames were loaded successfully, false otherwise.
	 * @param data          The value passed to SRCWRFloppy_LoadReplayCache.
	 * @param cache         The replay frame cache. May contain partially or zeroed data on failure.
	 */
	function void (bool loaded, any data, frame_cache_t cache);
	
	/**
	 * Called when SRCWRFloppy_LoadReplayCache has completed.
	 *
	 * @param loaded        True if frames were loaded successfully, false otherwise.
	 * @param data          The value passed to SRCWRFloppy_LoadReplayCache.
	 * @param cache         The replay frame cache. May contain partially or zeroed data on failure.
	 * @param header        The parsed replay header. May contain partially or zeroed data on failure.
	 */
	function void (bool loaded, any data, frame_cache_t cache, replay_header_t header);	
}

/**
 * Asynchronously saves a replay to one or more paths on a background thread.
 * Do NOT modify the `playerrecording` ArrayList until the callback fires... OR ELSE!!!!
 *
 * @param callback          Function to call when the save completes.
 * @param data              Arbitrary data forwarded to the callback.
 * @param paths             ArrayList of file paths to write the replay to.
 * @param header            Serialised replay header buffer.
 * @param headersize        Size of the header buffer in bytes.
 * @param playerrecording   ArrayList of recorded player frames.
 * @param totalframes       Total number of frames in playerrecording.
 */
native void SRCWRFloppy_AsyncSaveReplay(ReplaySavedCallback callback, any data, ArrayList paths, char[] header, int headersize, ArrayList playerrecording, int totalframes);

/**
 * Asynchronously saves a replay with an optional footer (extra per-frame module data) to one or more paths on a background thread. 
 * Build `footer` with WriteReplayFooterToBuffer. Pass `extraframes = null` / `totalextraframes = 0` (or `footersize = 0`) to write no footer, producing a byte-identical vanilla file.
 * Do NOT modify `playerrecording` or `extraframes` until the callback fires... OR ELSE!!!!
 *
 * @param callback          Function to call when the save completes.
 * @param data              Arbitrary data forwarded to the callback.
 * @param paths             ArrayList of file paths to write the replay to.
 * @param header            Serialised replay header buffer.
 * @param headersize        Size of the header buffer in bytes.
 * @param playerrecording   ArrayList of recorded player frames.
 * @param totalframes       Total number of frames in playerrecording.
 * @param footer            Serialised replay footer buffer.
 * @param footersize        Size of the footer buffer in bytes.
 * @param extraframes       ArrayList of extra per-frame data, or null for none.
 * @param totalextraframes  Total number of extra frames, or 0 for none.
 */
native void SRCWRFloppy_AsyncSaveReplayEx(ReplaySavedCallback callback, any data, ArrayList paths, char[] header, int headersize, ArrayList playerrecording, int totalframes, char[] footer, int footersize, ArrayList extraframes, int totalextraframes);

/**
 * Asynchronously loads replay frames (and optionally footer extra-frames) from a file on a background thread. 
 * The caller must pre-create the ArrayList(s) sized exactly to hold the data (new ArrayList(cellCount, frameCount)). 
 * The extension fills the backing stores off-thread and fires the callback when done.
 * Only REPLAY_FORMAT_FINAL binary replays are supported.
 * This continues even after a mapchange.
 * Do NOT modify `frames` or `extraframes` until the callback fires... OR ELSE!!!!
 *
 * @param callback          Function to call when loading completes.
 * @param data              Arbitrary data forwarded to the callback.
 * @param path              Path to the replay file.
 * @param frames            Pre-created ArrayList(frameCellCount, frameCount) to fill.
 * @param frameOffset       Byte offset in the file where frames begin.
 * @param frameCellCount    Cell size of one frame entry.
 * @param frameCount        Total number of frames to read.
 * @param extraframes       Optional pre-created ArrayList for footer extra-frames.
 * @param extraOffset       Byte offset in the file where extra frames begin.
 * @param extraCellCount    Cell size of one extra-frame entry.
 * @param extraCount        Total number of extra frames to read.
 */
native void SRCWRFloppy_AsyncLoadReplayFrames(ReplayFramesLoadedCallback callback, any data, const char[] path, ArrayList frames, int frameOffset, int frameCellCount, int frameCount, ArrayList extraframes = null, int extraOffset = 0, int extraCellCount = 0, int extraCount = 0);

/**
 * Convenience stock that reads the replay header synchronously, then loads all frames (and optional footer extra-frames) asynchronously. 
 * For non-final-format replays the frames are loaded synchronously via LoadReplayCache instead.
 * The callback receives the parsed header and a fully-populated frame_cache_t.
 * NOTE: Make sure the replay's map, style & track match what you expect or you use the optional style, track & mapname parameters.
 *
 * @param callback          Function to call when loading completes.
 * @param data              Arbitrary value forwarded to the callback.
 * @param path              Path to the replay file.
 * @param force_sync        True to force loading sync, false otherwise.
 * @param style             Expected replay style, -1 to accept any style.
 * @param track             Expected replay track, -1 to accept any track.
 * @param mapname           Expected map name (case-insensitive), leave empty to accept any map.
 * @noreturn
 */
stock void SRCWRFloppy_LoadReplayCache(LoadReplayCacheCallback callback, any data, const char[] path, bool force_sync = false, int style = -1, int track = -1, const char[] mapname = "")
{
	// Read header to get the required info 
	// TODO replace with ReadReplayHeaderAsync (ReadFileAsync) in SM 1.13? https://github.com/alliedmodders/sourcemod/pull/2524
	replay_header_t aHeader;
	File hFile = ReadReplayHeader(path, aHeader);
	if(hFile == null)
	{
		LogError("Cannot read replay header. '%s'", path);

		frame_cache_t aCache; // empty cache
		SRCWRFloppy_LoadReplayCache_StartFunction(callback, false, data, aCache, aHeader);
		return;
	}

	if(aHeader.iReplayVersion > REPLAY_FORMAT_SUBVERSION)
	{
		delete hFile;
		LogError("Replay version %d is newer than supported subversion %d. '%s'", aHeader.iReplayVersion, REPLAY_FORMAT_SUBVERSION, path);

		frame_cache_t aCache; // empty cache
		SRCWRFloppy_LoadReplayCache_StartFunction(callback, false, data, aCache, aHeader);
		return;
	}

	if((style != -1 && style != aHeader.iStyle) || (track != -1 && track != aHeader.iTrack) || (mapname[0] != '\0' && !StrEqual(mapname, aHeader.sMap, false)))
	{
		delete hFile;

		frame_cache_t aCache; // empty cache
		SRCWRFloppy_LoadReplayCache_StartFunction(callback, false, data, aCache, aHeader);
		return;
	}

	int iLastChange;
#if defined REPLAY_FOOTER_FORMAT
	iLastChange = GetFileTime(path, FileTime_LastChange);
#endif

	// Async loading only supports the modern binary format.
	if(!StrEqual(aHeader.sReplayFormat, REPLAY_FORMAT_FINAL) || force_sync)
	{
		delete hFile;

		frame_cache_t aCache;
		bool bSuccess = LoadReplayCache(aCache, aHeader.iStyle, aHeader.iTrack, path, aHeader.sMap);

		SRCWRFloppy_LoadReplayCache_StartFunction(callback, bSuccess, data, aCache, aHeader);
		return;
	}

	int iFrameOffset = hFile.Position; // frames start right after the header
	int iFrameCells = GetReplayCellSize(aHeader.iReplayVersion);
	int iFrameCount = aHeader.iFrameCount + aHeader.iPreFrames + aHeader.iPostFrames;

	ArrayList aFrames, aExtraFrames; 
	int iExtraFrameOffset, iExtraFrameCells, iExtraFrameCount;
#if defined REPLAY_FOOTER_FORMAT
	// Try to read optional footer
	int iFooterOffset = iFrameOffset + iFrameCount * iFrameCells * 4;
	int iFooterVersion;
	bool bFooter = ReadReplayFooterHeader(hFile, iFooterOffset, iFooterVersion, iExtraFrameCount, iExtraFrameOffset);

	iExtraFrameCells = bFooter ? GetReplayFooterCellSize(iFooterVersion) : 0; // cellsize inferred from subversion

	// Create the ArrayList's with a startsize equal to the total frame count
	aExtraFrames  = bFooter ? new ArrayList(iExtraFrameCells, iExtraFrameCount) : null;
#endif
	aFrames = new ArrayList(iFrameCells, iFrameCount);
	delete hFile;

	DataPack hPack = new DataPack();
	hPack.WriteFunction(callback);
	hPack.WriteCell(data);
	hPack.WriteCellArray(aHeader, sizeof(replay_header_t));
	hPack.WriteCell(aFrames);
	hPack.WriteCell(aExtraFrames);
	hPack.WriteCell(iLastChange);

	SRCWRFloppy_AsyncLoadReplayFrames(
		SRCWRFloppy_LoadReplayCache_Callback, hPack, path,
		aFrames, iFrameOffset, iFrameCells, iFrameCount,
		aExtraFrames, iExtraFrameOffset, iExtraFrameCells, iExtraFrameCount);
}

static void SRCWRFloppy_LoadReplayCache_Callback(bool loaded, any data)
{
	DataPack hPack = view_as<DataPack>(data);
	hPack.Reset();
	Function callback = hPack.ReadFunction();
	any callback_data = hPack.ReadCell();
	replay_header_t aHeader;
	hPack.ReadCellArray(aHeader, sizeof(replay_header_t));
	ArrayList aFrames = view_as<ArrayList>(hPack.ReadCell());
#if defined REPLAY_FOOTER_FORMAT
	ArrayList aExtraFrames = view_as<ArrayList>(hPack.ReadCell());
	int iLastChange = hPack.ReadCell();
#endif 
	delete hPack;

	if(!loaded)
	{
		delete aFrames;
#if defined REPLAY_FOOTER_FORMAT
		delete aExtraFrames;
#endif 
	}

	frame_cache_t aCache;
	aCache.iFrameCount = aHeader.iFrameCount;
	aCache.fTime = aHeader.fTime;
	aCache.bNewFormat = true;
	aCache.iReplayVersion = aHeader.iReplayVersion;
	aCache.sReplayName = "unknown";
	aCache.iPreFrames = aHeader.iPreFrames;
	aCache.aFrames = aFrames;
	aCache.iPostFrames = aHeader.iPostFrames;
	aCache.fTickrate = aHeader.fTickrate;
	aCache.iSteamID = aHeader.iSteamID;
#if defined REPLAY_FOOTER_FORMAT
	aCache.iLastChange = iLastChange; 
	aCache.aExtraFrames = aExtraFrames;
#endif 

	if(aCache.iSteamID != 0)
	{
		FormatEx(aCache.sReplayName, sizeof(aCache.sReplayName), "[U:1:%u]", aCache.iSteamID);
	}

	SRCWRFloppy_LoadReplayCache_StartFunction(callback, loaded, callback_data, aCache, aHeader);
}

static void SRCWRFloppy_LoadReplayCache_StartFunction(Function callback, bool loaded, any callback_data, frame_cache_t cache, replay_header_t header)
{
	Call_StartFunction(INVALID_HANDLE, callback);
	Call_PushCell(loaded);
	Call_PushCell(callback_data);
	Call_PushArray(cache, sizeof(cache));
	Call_PushArray(header, sizeof(header));
	Call_Finish();
}

#if !defined REPLAY_FOOTER_FORMAT
// Cells per replay frame for a given replay version
stock int GetReplayCellSize(int version)
{
	if (version >= 0x06) return 10;
	if (version >= 0x02) return 8;
	return 6;
}
#endif


public Extension __ext_srcwrfloppy =
{
	name = "srcwr💾",
	file = "srcwr💾.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_srcwrfloppy_SetNTVOptional()
{
	MarkNativeAsOptional("SRCWRFloppy_AsyncSaveReplay");
	MarkNativeAsOptional("SRCWRFloppy_AsyncSaveReplayEx");
	MarkNativeAsOptional("SRCWRFloppy_AsyncLoadReplayFrames");
}
#endif
