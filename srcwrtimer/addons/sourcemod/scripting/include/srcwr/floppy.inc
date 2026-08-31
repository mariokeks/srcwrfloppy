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
 * @param saved             True if the replay was saved successfully, false otherwise.
 * @param data              The value passed to SRCWRFloppy_AsyncSaveReplay[Ex].
 */
typedef ReplaySavedCallback = function void (bool saved, any data);

/**
 * Called when async replay frame loading has completed.
 *
 * @param loaded            True if frames were loaded successfully, false otherwise.
 * @param data              The value passed to SRCWRFloppy_AsyncLoadReplayFrames.
 */
typedef ReplayFramesLoadedCallback = function void (bool loaded, any data);

/**
 * Called when an async file read has completed.
 *
 * @param success           True if the file was opened, seeked & read successfully, false otherwise.
 * @param data              The value passed to SRCWRFloppy_ReadFileAsync.
 * @param path              The file path that was passed to SRCWRFloppy_ReadFileAsync.
 * @param buffer            The bytes read from the file.
 * @param bytesRead         Number of bytes actually copied into buffer (0 on failure).
 * @param totalFileSize     Total size of the file on disk in bytes (0 on failure).
 * @param lastModified      Unix timestamp of the file's last modification time (0 on failure).
 */
typedef ReadFileCallback = function void (bool success, any data, const char[] path, const char[] buffer, int bytesRead, int totalFileSize, int lastModified);

typeset LoadReplayCacheCallback
{
	/**
	 * Called when SRCWRFloppy_LoadReplayCache has completed.
	 *
	 * @param loaded           True if frames were loaded successfully, false otherwise.
	 * @param data             The value passed to SRCWRFloppy_LoadReplayCache.
	 * @param cache            The replay frame cache. May contain partially or zeroed data on failure.
	 */
	function void (bool loaded, any data, frame_cache_t cache);
	
	/**
	 * Called when SRCWRFloppy_LoadReplayCache has completed.
	 *
	 * @param loaded           True if frames were loaded successfully, false otherwise.
	 * @param data             The value passed to SRCWRFloppy_LoadReplayCache.
	 * @param cache            The replay frame cache. May contain partially or zeroed data on failure.
	 * @param header           The parsed replay header. May contain partially or zeroed data on failure.
	 */
	function void (bool loaded, any data, frame_cache_t cache, replay_header_t header);	
	
	/**
	 * Called when SRCWRFloppy_LoadReplayCache has completed.
	 *
	 * @param loaded           True if frames were loaded successfully, false otherwise.
	 * @param data             The value passed to SRCWRFloppy_LoadReplayCache.
	 * @param cache            The replay frame cache. May contain partially or zeroed data on failure.
	 * @param header           The parsed replay header. May contain partially or zeroed data on failure.
	 * @param footerVersion    The parsed footer version. 0 if footer is not available.
	 * @param extraFrameCount  The parsed extra frame count. 0 if footer is not available.
	 */
	function void (bool loaded, any data, frame_cache_t cache, replay_header_t header, int footerVersion, int extraFrameCount);	
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
 * Asynchronously reads a chunk of a file on a background thread.
 * This continues even after a mapchange.
 *
 * @param callback          Function to call when the read completes.
 * @param data              Arbitrary data forwarded to the callback.
 * @param path              Path to the file.
 * @param offset            Byte offset in the file to start reading from.
 * @param size              Number of bytes to read. 0 to read everything from offset to end of file.
 */
native void SRCWRFloppy_ReadFileAsync(ReadFileCallback callback, any data, const char[] path, int offset = 0, int size = 0);

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
	DataPack hPack = new DataPack();
	hPack.WriteFunction(callback);
	hPack.WriteCell(data);
	hPack.WriteCell(force_sync);
	hPack.WriteCell(style);
	hPack.WriteCell(track);
	hPack.WriteString(mapname);

	if(force_sync)
	{
		replay_header_t aHeader;
		File hFile = ReadReplayHeader(path, aHeader, style, track);
		bool bSuccess = (hFile != null);
		delete hFile;

		hPack.WriteCellArray(aHeader, sizeof(replay_header_t));

#if defined REPLAY_FOOTER_FORMAT
		ReadReplayHeader_Callback(bSuccess, hPack, path, "", 0, FileSize(path), GetFileTime(path, FileTime_LastChange));
#else
		ReadReplayHeader_Callback(bSuccess, hPack, path, "", 0, 0, 0);
#endif
	}
	else
	{
		SRCWRFloppy_ReadFileAsync(ReadReplayHeader_Callback, hPack, path, 0, 512);
	}
}

static void ReadReplayHeader_Callback(bool success, DataPack data, const char[] path, const char[] buffer, int bytesRead, int totalFileSize, int lastModified)
{
	data.Reset();
	Function callback = data.ReadFunction();
	any callback_data = data.ReadCell();
	bool bForceSync = view_as<bool>(data.ReadCell());
	int iStyle = data.ReadCell();
	int iTrack = data.ReadCell();
	char sMap[PLATFORM_MAX_PATH]; data.ReadString(sMap, sizeof(sMap));
	replay_header_t aHeader;
	if(bForceSync)
	{
		data.ReadCellArray(aHeader, sizeof(replay_header_t));
	}
	delete data;

	int iFrameOffset; // frames start right after the header, so after SRCWRFloppy_ReadReplayHeaderFromBuffer

	if(!success || (!bForceSync && (iFrameOffset = SRCWRFloppy_ReadReplayHeaderFromBuffer(buffer, bytesRead, totalFileSize, aHeader, iStyle, iTrack)) == -1))
	{
		LogError("Cannot read replay header. '%s'", path);

		frame_cache_t aCache; // empty cache
		SRCWRFloppy_LoadReplayCache_StartFunction(callback, false, callback_data, aCache, aHeader);
		return;
	}

	if(aHeader.iReplayVersion > REPLAY_FORMAT_SUBVERSION)
	{
		LogError("Replay version %d is newer than supported subversion %d. '%s'", aHeader.iReplayVersion, REPLAY_FORMAT_SUBVERSION, path);

		frame_cache_t aCache; // empty cache
		SRCWRFloppy_LoadReplayCache_StartFunction(callback, false, callback_data, aCache, aHeader);
		return;
	}

	if((iStyle != -1 && iStyle != aHeader.iStyle) || (iTrack != -1 && iTrack != aHeader.iTrack) || (sMap[0] != '\0' && !StrEqual(sMap, aHeader.sMap, false)))
	{
		frame_cache_t aCache; // empty cache
		SRCWRFloppy_LoadReplayCache_StartFunction(callback, false, callback_data, aCache, aHeader);
		return;
	}

	// Async loading only supports the modern binary format.
	if(!StrEqual(aHeader.sReplayFormat, REPLAY_FORMAT_FINAL) || bForceSync)
	{
		frame_cache_t aCache;
		bool bSuccess = LoadReplayCache(aCache, aHeader.iStyle, aHeader.iTrack, path, aHeader.sMap);

		SRCWRFloppy_LoadReplayCache_StartFunction(callback, bSuccess, callback_data, aCache, aHeader);
		return;
	}

#if defined REPLAY_FOOTER_FORMAT
	MaybeReadFooter(callback, callback_data, path, aHeader, lastModified, iFrameOffset, totalFileSize);
#else
	LoadReplayFrames(callback, callback_data, path, aHeader, lastModified, iFrameOffset);
#endif
}

static void LoadReplayFrames(Function callback, any data, const char[] path, replay_header_t header, int lastModified, int frameOffset, int extraFrameOffset = 0, int footerVersion = 0, int extraFrameCells = 0, int extraFrameCount = 0)
{
	int iFrameCells = GetReplayCellSize(header.iReplayVersion);
	int iFrameCount = header.iFrameCount + header.iPreFrames + header.iPostFrames;
	ArrayList aFrames = new ArrayList(iFrameCells, iFrameCount); 

	ArrayList aExtraFrames;
	if(footerVersion > 0)
	{
		aExtraFrames = new ArrayList(extraFrameCells, extraFrameCount); 
	}

	DataPack hPack = new DataPack();
	hPack.WriteFunction(callback);
	hPack.WriteCell(data);
	hPack.WriteCellArray(header, sizeof(replay_header_t));
	hPack.WriteCell(aFrames);
	hPack.WriteCell(aExtraFrames);
	hPack.WriteCell(footerVersion);
	hPack.WriteCell(extraFrameCount);
	hPack.WriteCell(lastModified);

	SRCWRFloppy_AsyncLoadReplayFrames(
		SRCWRFloppy_LoadReplayCache_Callback, hPack, path,
		aFrames, frameOffset, iFrameCells, iFrameCount,
		aExtraFrames, extraFrameOffset, extraFrameCells, extraFrameCount);
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
	ArrayList aExtraFrames = view_as<ArrayList>(hPack.ReadCell());
	int iFooterVersion = hPack.ReadCell();
	int iExtraFrameCount = hPack.ReadCell();
#if defined REPLAY_FOOTER_FORMAT
	int iLastChange = hPack.ReadCell();
#endif 
	delete hPack;

	if(!loaded)
	{
		delete aFrames;
		delete aExtraFrames;
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

	SRCWRFloppy_LoadReplayCache_StartFunction(callback, loaded, callback_data, aCache, aHeader, iFooterVersion, iExtraFrameCount);
}

static void SRCWRFloppy_LoadReplayCache_StartFunction(Function callback, bool loaded, any callback_data, frame_cache_t cache, replay_header_t header, int footerVersion = 0, int extraFrameCount = 0)
{
	Call_StartFunction(INVALID_HANDLE, callback);
	Call_PushCell(loaded);
	Call_PushCell(callback_data);
	Call_PushArray(cache, sizeof(cache));
	Call_PushArray(header, sizeof(header));
	Call_PushCell(footerVersion);
	Call_PushCell(extraFrameCount);
	Call_Finish();
}

stock int SRCWRFloppy_ReadReplayHeaderFromBuffer(const char[] buffer, int bytesRead, int totalFileSize, replay_header_t header, int style = 0, int track = 0)
{
	replay_header_t empty_header;
	header = empty_header;

	int iPos;
	char sHeader[64];
	if (!Buffer_ReadLine(buffer, bytesRead, iPos, sHeader, sizeof(sHeader)))
		return -1;

	TrimString(sHeader);
	char sExplodedHeader[2][64];
	ExplodeString(sHeader, ":", sExplodedHeader, sizeof(sExplodedHeader), sizeof(sExplodedHeader[]));

	strcopy(header.sReplayFormat, sizeof(header.sReplayFormat), sExplodedHeader[1]);

	if (StrEqual(header.sReplayFormat, REPLAY_FORMAT_FINAL)) // hopefully, the last of them
	{
		header.iReplayVersion = StringToInt(sExplodedHeader[0]);

		// replay file integrity and PreFrames
		if (header.iReplayVersion >= 0x03)
		{
			if (!Buffer_ReadString(buffer, bytesRead, iPos, header.sMap, sizeof(header.sMap)))
				return -1;
			if (!Buffer_ReadUint8(buffer, bytesRead, iPos, header.iStyle))
				return -1;
			if (!Buffer_ReadUint8(buffer, bytesRead, iPos, header.iTrack))
				return -1;

			if (!Buffer_ReadInt32(buffer, bytesRead, iPos, header.iPreFrames))
				return -1;

			// In case the replay was from when there could still be negative preframes
			if (header.iPreFrames < 0)
			{
				header.iPreFrames = 0;
			}
		}

		if (!Buffer_ReadInt32(buffer, bytesRead, iPos, header.iFrameCount))
			return -1;
		if (!Buffer_ReadInt32(buffer, bytesRead, iPos, view_as<int>(header.fTime)))
			return -1;

		if (header.iReplayVersion < 0x07)
		{
			header.iFrameCount -= header.iPreFrames;
		}

		if (header.iReplayVersion >= 0x04)
		{
			if (!Buffer_ReadInt32(buffer, bytesRead, iPos, header.iSteamID))
				return -1;
		}
		else
		{
			char sAuthID[32];
			if (!Buffer_ReadString(buffer, bytesRead, iPos, sAuthID, sizeof(sAuthID)))
				return -1;
			ReplaceString(sAuthID, sizeof(sAuthID), "[U:1:", "");
			ReplaceString(sAuthID, sizeof(sAuthID), "]", "");
			header.iSteamID = StringToInt(sAuthID);
		}

		if (header.iReplayVersion >= 0x05)
		{
			if (!Buffer_ReadInt32(buffer, bytesRead, iPos, header.iPostFrames))
				return -1;
			if (!Buffer_ReadInt32(buffer, bytesRead, iPos, view_as<int>(header.fTickrate)))
				return -1;

			if (header.iReplayVersion < 0x07)
			{
				header.iFrameCount -= header.iPostFrames;
			}
		}

		if (header.iReplayVersion >= 0x08)
		{
			if (!Buffer_ReadInt32(buffer, bytesRead, iPos, view_as<int>(header.fZoneOffset[0])))
				return -1;
			if (!Buffer_ReadInt32(buffer, bytesRead, iPos, view_as<int>(header.fZoneOffset[1])))
				return -1;
		}

		if (header.iReplayVersion >= 0x0c)
		{
			if (!Buffer_ReadInt32(buffer, bytesRead, iPos, header.iTimestamp))
				return -1;
		}
	}
	else if (StrEqual(header.sReplayFormat, REPLAY_FORMAT_V2))
	{
		header.iFrameCount = StringToInt(sExplodedHeader[0]);
	}
	else // old, outdated and slow - only used for ancient replays
	{
		// check for btimes replays
		iPos = 0;

		int btimes_player_id;
		if (!Buffer_ReadInt32(buffer, bytesRead, iPos, btimes_player_id))
			return -1;

		float run_time;
		if (!Buffer_ReadInt32(buffer, bytesRead, iPos, view_as<int>(run_time)))
			return -1;

		if (btimes_player_id >= 0 && run_time > 0.0 && run_time < (10.0 * 60.0 * 60.0))
		{
			header.sReplayFormat = "btimes";
			header.fTime = run_time;

			// We don't have the whole file (or a SEEK_END-able File) here, so use totalFileSize instead of file.Position.
			if (totalFileSize >= 0)
			{
				header.iFrameCount = (totalFileSize / 4 - 2) / 6;
			}
		}
		else
		{
			return -1;
		}
	}

	if (header.iReplayVersion < 0x03)
	{
		header.iStyle = style;
		header.iTrack = track;
	}

	if (header.iReplayVersion < 0x05)
	{
		header.fTickrate = (1.0 / GetTickInterval()); // just assume it's our own tickrate...
	}
	
	return iPos;
}

static bool Buffer_ReadUint8(const char[] buffer, int bytesRead, int &pos, int &out)
{
	if (pos < 0 || pos >= bytesRead)
	{
		return false;
	}

	out = buffer[pos++] & 0xFF;
	return true;
}

static bool Buffer_ReadInt32(const char[] buffer, int bytesRead, int &pos, int &out)
{
	if (pos < 0 || bytesRead - pos < 4)
	{
		return false;
	}

	out = (buffer[pos] & 0xFF)
		| ((buffer[pos + 1] & 0xFF) << 8)
		| ((buffer[pos + 2] & 0xFF) << 16)
		| ((buffer[pos + 3] & 0xFF) << 24);

	pos += 4;
	return true;
}

static bool Buffer_ReadLine(const char[] buffer, int bytesRead, int &pos, char[] out, int maxlen)
{
	if (maxlen <= 0 || pos < 0)
	{
		return false;
	}

	out[0] = '\0';

	int i = 0;

	while (pos < bytesRead)
	{
		char c = buffer[pos++];

		if (c == '\n')
		{
			out[i] = '\0';
			return true;
		}

		if (c == '\r')
		{
			if (pos < bytesRead && buffer[pos] == '\n')
			{
				pos++;
			}

			out[i] = '\0';
			return true;
		}

		if (i < maxlen - 1)
		{
			out[i++] = c;
		}
	}

	out[i] = '\0';
	return false;
}

static bool Buffer_ReadString(const char[] buffer, int bytesRead, int &pos, char[] out, int maxlen)
{
	if (maxlen <= 0 || pos < 0)
	{
		return false;
	}

	int i = 0;

	while (pos < bytesRead)
	{
		char c = buffer[pos++];

		if (c == '\0')
		{
			out[i < maxlen ? i : maxlen - 1] = '\0';
			return true;
		}

		if (i < maxlen - 1)
		{
			out[i++] = c;
		}
	}

	out[i < maxlen ? i : maxlen - 1] = '\0';
	return false; // ran out of bytes before finding a NUL
}

#if defined REPLAY_FOOTER_FORMAT
stock int SRCWRFloppy_ReadReplayFooterHeaderFromBuffer(const char[] buffer, int bytesRead, int &subversion, int &extraFrameCount)
{
	int iPos;

	char sHeader[64];
	if (!Buffer_ReadLine(buffer, bytesRead, iPos, sHeader, sizeof(sHeader)))
		return -1;

	TrimString(sHeader);
	if(!StrEqual(sHeader, REPLAY_FOOTER_FORMAT))
		return -1; // something else??

	if (!Buffer_ReadUint8(buffer, bytesRead, iPos, subversion) || subversion > REPLAY_FOOTER_SUBVERSION)
		return -1; // Not going to try and read a newer version

	if (!Buffer_ReadInt32(buffer, bytesRead, iPos, extraFrameCount))
		return -1;

	if(extraFrameCount <= 0)
		return -1;

	return iPos;
}

static void MaybeReadFooter(Function callback, any data, const char[] path, replay_header_t header, int lastModified, int frameOffset, int totalFileSize)
{
	int iFooterOffset = frameOffset + (header.iFrameCount + header.iPreFrames + header.iPostFrames) * GetReplayCellSize(header.iReplayVersion) * 4;

	if(iFooterOffset >= totalFileSize)
	{	
		LoadReplayFrames(callback, data, path, header, lastModified, frameOffset);
	}
	else
	{
		DataPack hPack = new DataPack();
		hPack.WriteFunction(callback);
		hPack.WriteCell(data);
		hPack.WriteString(path);
		hPack.WriteCellArray(header, sizeof(replay_header_t));
		hPack.WriteCell(frameOffset);

		SRCWRFloppy_ReadFileAsync(ReadReplayFooterHeader_Callback, hPack, path, iFooterOffset, REPLAY_FOOTER_MAX_HEADER_SIZE);
	}
}

static void ReadReplayFooterHeader_Callback(bool success, DataPack data, const char[] path, const char[] buffer, int bytesRead, int totalFileSize, int lastModified)
{
	data.Reset();
	Function callback = data.ReadFunction();
	any callback_data = data.ReadCell();
	char sPath[PLATFORM_MAX_PATH]; data.ReadString(sPath, sizeof(sPath));
	replay_header_t aHeader; data.ReadCellArray(aHeader, sizeof(replay_header_t));
	int iFrameOffset = data.ReadCell();
	delete data;

	int iFooterVersion, iExtraFrameCount, iExtraFrameOffset;

	if(!success || (iExtraFrameOffset = SRCWRFloppy_ReadReplayFooterHeaderFromBuffer(buffer, bytesRead, iFooterVersion, iExtraFrameCount)) == -1)
	{
		LogError("Cannot read replay footer header. '%s'", path);

		frame_cache_t aCache; // empty cache
		SRCWRFloppy_LoadReplayCache_StartFunction(callback, false, callback_data, aCache, aHeader);
		return;
	}

	int iExtraFrameCells = iFooterVersion ? GetReplayFooterCellSize(iFooterVersion) : 0;

	LoadReplayFrames(callback, callback_data, sPath, aHeader, lastModified, iFrameOffset, iExtraFrameOffset, iFooterVersion, iExtraFrameCells, iExtraFrameCount);
}
#else
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
	MarkNativeAsOptional("SRCWRFloppy_ReadFileAsync");
}
#endif
