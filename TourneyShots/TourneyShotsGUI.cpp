#include "pch.h"
#include "TourneyShots.h"

void TourneyShots::SetImGuiContext(uintptr_t ctx) {
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

std::string TourneyShots::GetPluginName() {
	return "Tourney Shots";
}

void TourneyShots::RenderSettings() {
	CVarWrapper enableCvar = cvarManager->getCvar("tourney_only_if_tied");
	if (!enableCvar) { return; }
	bool enabled = enableCvar.getBoolValue();
	if (ImGui::Checkbox("Only display if tied", &enabled)) {
		enableCvar.setValue(enabled);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Toggle display if tied");
	}

	enableCvar = cvarManager->getCvar("tourney_only_if_tournament");
	if (!enableCvar) { return; }
	enabled = enableCvar.getBoolValue();
	if (ImGui::Checkbox("Only display if in a tournament", &enabled)) {
		enableCvar.setValue(enabled);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Toggle display if in tournament");
	}
}