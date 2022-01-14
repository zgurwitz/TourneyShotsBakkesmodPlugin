#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class TourneyShots: public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{

	virtual void onLoad();
	virtual void onUnload();
	void LoadHooks();
	void InitTeam();
	void Reset();
	void GameTied();
	void Render(CanvasWrapper canvas);
	void onStatTickerMessage(void* params);
	struct StatTickerParams {
		// person who got a stat
		uintptr_t Receiver;
		// person who is victim of a stat (only exists for demos afaik)
		uintptr_t Victim;
		// wrapper for the stat event
		uintptr_t StatEvent;
	};
	void RenderSettings() override;
	std::string GetPluginName() override;
	void SetImGuiContext(uintptr_t ctx) override;
private:
	void Log(std::string msg);
};

