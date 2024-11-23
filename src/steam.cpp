#include "steam.h"
#include "utils.h"
#include "log.h"

#include <libloaderapi.h>
#include <sysinfoapi.h>

HMODULE hDll = nullptr;

typedef bool(*LPRESTARTAPPIFNECESSARY)(uint32);
typedef bool(*LPINIT)();
typedef void(*LPRUNCALLBACKS)();
typedef void(*LPSHUTDOWN)();
typedef void(*LPREGISTERCALLBACK)(class CCallbackBase *pCallback, int iCallback);
typedef void(*LPUNREGISTERCALLBACK)(class CCallbackBase *pCallback);
typedef ISteamUser*(*LPUSER)();
typedef ISteamUtils*(*LPUTILS)();
typedef ISteamUserStats*(*LPUSERSTATS)();

bool load_library()
{
	if (hDll != nullptr) {
		return true;
	}

	char lib_path[MAX_PATH] = {};

	snprintf(lib_path, sizeof(lib_path), R"(%s\ffnx_steam_api.dll)", basedir);

	if (!fileExists(lib_path)) {
		snprintf(lib_path, sizeof(lib_path), R"(%s\steam_api.dll)", basedir);
	}

	hDll = LoadLibraryA(lib_path);

	if (hDll != nullptr) {
		return true;
	}

	ffnx_error("%s: cannot load library %s\n", __func__, lib_path);

	return false;
}

bool steam_api_restart_app_if_necessary(uint32 unOwnAppID)
{
	if (!load_library()) {
		return false;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamAPI_RestartAppIfNecessary");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return false;
	}

	return LPRESTARTAPPIFNECESSARY(proc)(unOwnAppID);
}

bool steam_api_init()
{
	if (!load_library()) {
		return false;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamAPI_Init");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return false;
	}

	return LPINIT(proc)();
}

void steam_api_run_callbacks()
{
	if (!load_library()) {
		return;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamAPI_RunCallbacks");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return;
	}

	return LPRUNCALLBACKS(proc)();
}

void steam_api_shutdown()
{
	if (!load_library()) {
		return;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamAPI_Shutdown");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return;
	}

	return LPSHUTDOWN(proc)();
}

void steam_api_register_callback(class CCallbackBase *pCallback, int iCallback)
{
	if (!load_library()) {
		return;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamAPI_RegisterCallback");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return;
	}

	return LPREGISTERCALLBACK(proc)(pCallback, iCallback);
}

void steam_api_unregister_callback(class CCallbackBase *pCallback)
{
	if (!load_library()) {
		return;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamAPI_UnregisterCallback");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return;
	}

	return LPUNREGISTERCALLBACK(proc)(pCallback);
}

ISteamUser *steam_user()
{
	if (!load_library()) {
		return nullptr;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamUser");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return nullptr;
	}

	return LPUSER(proc)();
}

ISteamUtils *steam_utils()
{
	if (!load_library()) {
		return nullptr;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamUtils");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return nullptr;
	}

	return LPUTILS(proc)();
}

ISteamUserStats *steam_user_stats()
{
	if (!load_library()) {
		return nullptr;
	}

	FARPROC proc = GetProcAddress(hDll, "SteamUserStats");
	if (proc == nullptr) {
		ffnx_error("%s: Function not found in Steam lib\n", __func__);

		return nullptr;
	}

	return LPUSERSTATS(proc)();
}
