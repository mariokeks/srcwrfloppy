#include <profiler>

#include <shavit/replay-playback>	

#undef REQUIRE_EXTENSIONS
#include <srcwr/floppy>

bool gB_FloppyAsyncLoad;
bool gB_Profiling;

public void OnPluginStart()
{
	RegAdminCmd("sm_testload", Command_PlayReplayFile, ADMFLAG_RCON, "Loads a replay file, prints load time and thread-block time. Usage: sm_testload <path>");

	gB_FloppyAsyncLoad = (GetFeatureStatus(FeatureType_Native, "SRCWRFloppy_AsyncLoadReplayFrames") == FeatureStatus_Available);
}

public void OnLibraryAdded(const char[] name)
{
	if(strcmp(name, "srcwr💾") == 0)
	{
		gB_FloppyAsyncLoad = (GetFeatureStatus(FeatureType_Native, "SRCWRFloppy_AsyncLoadReplayFrames") == FeatureStatus_Available);
	}
}

public void OnLibraryRemoved(const char[] name)
{
	if(strcmp(name, "srcwr💾") == 0)
	{
		gB_FloppyAsyncLoad = false;
	}
}

Action Command_PlayReplayFile(int client, int args)
{
	if(gB_Profiling)
	{
		PrintToChat(client, "Please wait a moment.");
		return Plugin_Handled;
	}

	if(args == 0)
	{
		PrintToChat(client, "Usage: sm_testload <path>");
		return Plugin_Handled;
	}

	char sPath[PLATFORM_MAX_PATH];
	GetCmdArgString(sPath, sizeof(sPath));

	if(!FileExists(sPath))
	{
		PrintToChat(client, "Replay file not found. (%s)", sPath);
		return Plugin_Handled;
	}

	if(gB_FloppyAsyncLoad)
	{
		PrintToChat(client, "Async loading.. (First load may take longer)");
	}
	else
	{
		PrintToChat(client, "Sync loading.. ");
	}

	Profiler profGameThread = new Profiler();
	Profiler profTotal = new Profiler();
	profGameThread.Start();
	profTotal.Start();

	DataPack hPack = new DataPack();
	hPack.WriteCell(GetClientSerial(client));
	hPack.WriteCell(profGameThread);
	hPack.WriteCell(profTotal);

	gB_Profiling = true;

	SRCWRFloppy_LoadReplayCache(ReplayLoaded_Callback, hPack, sPath, !gB_FloppyAsyncLoad);

	if(gB_Profiling)
	{
		profGameThread.Stop();
		PrintToChat(client, "Game thread blocked for %fs", profGameThread.Time);
		delete profGameThread;
		gB_Profiling = false;
	}

	return Plugin_Handled;
}

void ReplayLoaded_Callback(bool loaded, DataPack data, frame_cache_t cache, replay_header_t header)
{
	data.Reset();
	int client = GetClientFromSerial(data.ReadCell());
	Profiler profGameThread = view_as<Profiler>(data.ReadCell());
	Profiler profTotal = view_as<Profiler>(data.ReadCell());
	delete data;
	
	profTotal.Stop();

	// We dont want to include everything from the callback 
	if(gB_Profiling)
	{
		profGameThread.Stop();
		if(client)
		{
			PrintToChat(client, "Game thread blocked for %fs", profGameThread.Time);
		}
		delete profGameThread;
		gB_Profiling = false;
	}

	float fTimeTotal = profTotal.Time;
	delete profTotal;

	if(!client)
	{
		delete cache.aFrames;
		return;
	}

	if(!loaded)
	{
		delete cache.aFrames;
		PrintToChat(client, "Failed to load replay.");
		return;
	}

	if(Shavit_StartReplayFromFrameCache(header.iStyle, header.iTrack, -1.0, client, -1, Replay_Dynamic, false, cache) == 0)
	{
		PrintToChat(client, "Failed to create replay bot.");
		return;
	}

	delete cache.aFrames;
	
	// First load with the extension may take much longer
	PrintToChat(client, "Loaded in %fs", fTimeTotal);
}
