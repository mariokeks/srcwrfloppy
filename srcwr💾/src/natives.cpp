// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 rtldg <rtldg@protonmail.com>

#include "../../../srcwrtimer/extshared/src/extension.h"
#include "../../../srcwrtimer/extshared/src/coreident.hpp"
#include <ICellArray.h>


extern "C" {

void rust_setup_replay_thread();
void rust_KILL_replay_thread();

void rust_post_to_replay_thread(
	  IChangeableForward* forward // what to pass along to the callback
	, int value // what to pass along to the callback
	, SourceMod::ICellArray* paths
	, const char* header
	, size_t headersize
	, void* playerrecording
	, size_t totalframes
);

void rust_post_to_replay_thread_ex(
	  IChangeableForward* forward // what to pass along to the callback
	, int value // what to pass along to the callback
	, SourceMod::ICellArray* paths
	, const char* header
	, size_t headersize
	, void* playerrecording
	, size_t totalframes
	, const char* footer
	, size_t footersize
	, void* extrarecording // may be null => no footer
	, size_t totalextraframes
);

void rust_post_load_to_replay_thread(
	  IChangeableForward* forward // what to pass along to the callback
	, int value // what to pass along to the callback
	, const char* path
	, void* frames // pre-sized ArrayList backing store to fill
	, int frameoffset
	, int framecellcount
	, int framecount
	, void* extraframes // may be null => load frames only
	, int extraoffset
	, int extracellcount
	, int extracount
);

void rust_post_read_file_to_thread(
	  IChangeableForward* forward // what to pass along to the callback
	, int value // what to pass along to the callback
	, const char* path
	, int offset
	, int size // <= 0 means "read to end of file starting at offset"
);

}


extern const sp_nativeinfo_t FloppyNatives[];


void MyExtension::OnHandleDestroy(HandleType_t type, void* object) {}
bool MyExtension::GetHandleApproxSize(HandleType_t type, void* object, unsigned int* size) { return false; }


bool Extension_OnLoad(char* error, size_t maxlength)
{
	rust_setup_replay_thread();

	sharesys->AddNatives(myself, FloppyNatives);
	return true;
}

void Extension_OnUnload()
{
	rust_KILL_replay_thread();
}

void Extension_OnAllLoaded() {}

static cell_t N_SRCWRFloppy_AsyncSaveReplay(IPluginContext* ctx, const cell_t* params)
{
	int p = 1;
	cell_t callback = params[p++];
	int value = params[p++];

	ICellArray* paths;
	Handle_t paths_handle = params[p++];
	if (HandleError err = ReadHandleCoreIdent(paths_handle, g_ArrayListType, (void**)&paths); err != HandleError_None)
		return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", paths_handle, err);

	char* header;
	(void)ctx->LocalToString(params[p++], &header);
	int headersize = params[p++];

	void* playerrecording;
	Handle_t playerrecording_handle = params[p++];
	if (HandleError err = ReadHandleCoreIdent(playerrecording_handle, g_ArrayListType, &playerrecording); err != HandleError_None)
		return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", playerrecording_handle, err);

	int totalframes = params[p++];

	IChangeableForward* fw = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 2
		, NULL
		, Param_Any // saved
		, Param_Any // value
	);
	if (!fw || !fw->AddFunction(ctx, callback))
	{
		if (fw) forwards->ReleaseForward(fw);
		return ctx->ThrowNativeError("Failed to create callback forward");
	}

	rust_post_to_replay_thread(
		  fw
		, value
		, paths
		, header
		, headersize
		, playerrecording
		, totalframes
	);

	return 0; // native marked as void so return value doesn't matter...
}

static cell_t N_SRCWRFloppy_AsyncSaveReplayEx(IPluginContext* ctx, const cell_t* params)
{
	int p = 1;
	cell_t callback = params[p++];
	int value = params[p++];

	ICellArray* paths;
	Handle_t paths_handle = params[p++];
	if (HandleError err = ReadHandleCoreIdent(paths_handle, g_ArrayListType, (void**)&paths); err != HandleError_None)
		return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", paths_handle, err);

	char* header;
	(void)ctx->LocalToString(params[p++], &header);
	int headersize = params[p++];

	void* playerrecording;
	Handle_t playerrecording_handle = params[p++];
	if (HandleError err = ReadHandleCoreIdent(playerrecording_handle, g_ArrayListType, &playerrecording); err != HandleError_None)
		return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", playerrecording_handle, err);

	int totalframes = params[p++];

	char* footer;
	(void)ctx->LocalToString(params[p++], &footer);
	int footersize = params[p++];

	// extraframes is optional: a null handle means "no footer".
	void* extrarecording = NULL;
	Handle_t extrarecording_handle = params[p++];
	if (extrarecording_handle != 0)
	{
		if (HandleError err = ReadHandleCoreIdent(extrarecording_handle, g_ArrayListType, &extrarecording); err != HandleError_None)
			return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", extrarecording_handle, err);
	}

	int totalextraframes = params[p++];

	IChangeableForward* fw = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 2
		, NULL
		, Param_Any // saved
		, Param_Any // value
	);
	if (!fw || !fw->AddFunction(ctx, callback))
	{
		if (fw) forwards->ReleaseForward(fw);
		return ctx->ThrowNativeError("Failed to create callback forward");
	}

	rust_post_to_replay_thread_ex(
		  fw
		, value
		, paths
		, header
		, headersize
		, playerrecording
		, totalframes
		, footer
		, footersize
		, extrarecording
		, totalextraframes
	);

	return 0; // native marked as void so return value doesn't matter...
}

static cell_t N_SRCWRFloppy_AsyncLoadReplayFrames(IPluginContext* ctx, const cell_t* params)
{
	int p = 1;
	cell_t callback = params[p++];
	int value = params[p++];

	char* path;
	(void)ctx->LocalToString(params[p++], &path);

	void* frames;
	Handle_t frames_handle = params[p++];
	if (HandleError err = ReadHandleCoreIdent(frames_handle, g_ArrayListType, &frames); err != HandleError_None)
		return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", frames_handle, err);

	int frameoffset = params[p++];
	int framecellcount = params[p++];
	int framecount = params[p++];

	// extraframes is optional: a null handle means "load frames only".
	void* extraframes = NULL;
	Handle_t extraframes_handle = params[p++];
	if (extraframes_handle != 0)
	{
		if (HandleError err = ReadHandleCoreIdent(extraframes_handle, g_ArrayListType, &extraframes); err != HandleError_None)
			return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", extraframes_handle, err);
	}

	int extraoffset = params[p++];
	int extracellcount = params[p++];
	int extracount = params[p++];

	IChangeableForward* fw = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 2
		, NULL
		, Param_Any // loaded
		, Param_Any // value
	);
	if (!fw || !fw->AddFunction(ctx, callback))
	{
		if (fw) forwards->ReleaseForward(fw);
		return ctx->ThrowNativeError("Failed to create callback forward");
	}

	rust_post_load_to_replay_thread(
		  fw
		, value
		, path
		, frames
		, frameoffset
		, framecellcount
		, framecount
		, extraframes
		, extraoffset
		, extracellcount
		, extracount
	);

	return 0; // native marked as void so return value doesn't matter...
}

static cell_t N_SRCWRFloppy_ReadFileAsync(IPluginContext* ctx, const cell_t* params)
{
	int p = 1;
	cell_t callback = params[p++];
	int value = params[p++];

	char* path;
	(void)ctx->LocalToString(params[p++], &path);

	int offset = params[p++];
	int size = params[p++];

	IChangeableForward* fw = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 7
		, NULL
		, Param_Cell   // success
		, Param_Any    // data
		, Param_String // path
		, Param_String // buffer (binary-safe; use bytesRead as the real length, not strlen)
		, Param_Cell   // bytesRead
		, Param_Cell   // totalFileSize
		, Param_Cell   // lastModified
	);
	if (!fw || !fw->AddFunction(ctx, callback))
	{
		if (fw) forwards->ReleaseForward(fw);
		return ctx->ThrowNativeError("Failed to create callback forward");
	}

	rust_post_read_file_to_thread(
		  fw
		, value
		, path
		, offset
		, size
	);

	return 0; // native marked as void so return value doesn't matter...
}

extern const sp_nativeinfo_t FloppyNatives[] = {
	{"SRCWRFloppy_AsyncSaveReplay", N_SRCWRFloppy_AsyncSaveReplay},
	{"SRCWRFloppy_AsyncSaveReplayEx", N_SRCWRFloppy_AsyncSaveReplayEx},
	{"SRCWRFloppy_AsyncLoadReplayFrames", N_SRCWRFloppy_AsyncLoadReplayFrames},
	{"SRCWRFloppy_ReadFileAsync", N_SRCWRFloppy_ReadFileAsync},
	{NULL, NULL}
};
