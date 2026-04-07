/**
 * @file
 */

#include "ScriptBrowserPanel.h"
#include "app/I18N.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"

namespace voxelui {

static const char *SCRIPT_API_URL = "https://vengi-voxel.de/api";

void ScriptBrowserPanel::open() {
	_open = true;
	_requestFocus = true;
	if (_scripts.empty() && !_requestPending) {
		fetchScripts();
	}
}

void ScriptBrowserPanel::fetchScripts() {
	_requestPending = true;
	voxelui::ScriptApi api;
	_scripts = api.query(SCRIPT_API_URL);
	_requestPending = false;
}

bool ScriptBrowserPanel::needsReload() {
	if (_needsReload) {
		_needsReload = false;
		return true;
	}
	return false;
}

bool ScriptBrowserPanel::filtered(const voxelui::ScriptInfo &info) const {
	if (_searchFilter.empty()) {
		return false;
	}
	if (core::string::icontains(info.name, _searchFilter)) {
		return false;
	}
	if (core::string::icontains(info.description, _searchFilter)) {
		return false;
	}
	if (core::string::icontains(info.author, _searchFilter)) {
		return false;
	}
	return true;
}

bool ScriptBrowserPanel::isInstalled(const voxelui::ScriptInfo &info) const {
	core::String dir;
	if (info.type == "generator") {
		dir = "scripts";
	} else if (info.type == "brush") {
		dir = "brushes";
	} else if (info.type == "selectionmode") {
		dir = "selectionmodes";
	} else {
		return false;
	}
	const core::String path = _app->filesystem()->homeWritePath(core::string::path(dir, info.filename));
	return io::Filesystem::sysExists(path);
}

void ScriptBrowserPanel::update(const char *id) {
	if (!_open) {
		return;
	}

	const core::String title = makeTitle(ICON_LC_DOWNLOAD, _("Script Browser"), id);
	if (_requestFocus) {
		ImGui::SetNextWindowFocus();
		_requestFocus = false;
	}
	if (ImGui::Begin(title.c_str(), &_open)) {
		if (_requestPending) {
			ImGui::Spinner("fetching_scripts", ImGui::Size(1.0f));
			ImGui::SameLine();
			ImGui::TextUnformatted(_("Fetching scripts..."));
		} else {
			if (ImGui::IconButton(ICON_LC_LOADER_CIRCLE, _("Refresh"))) {
				fetchScripts();
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##scriptfilter", &_searchFilter);
			ImGui::TooltipTextUnformatted(_("Filter scripts by name, description or author"));

			if (_scripts.empty()) {
				ImGui::TextUnformatted(_("No scripts available"));
			} else {
				const ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders |
												   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
												   ImGuiTableFlags_Sortable;
				if (ImGui::BeginTable("##scriptbrowser", 6, tableFlags)) {
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_DefaultSort);
					ImGui::TableSetupColumn(_("Type"));
					ImGui::TableSetupColumn(_("Author"));
					ImGui::TableSetupColumn(_("Version"));
					ImGui::TableSetupColumn(_("Installed"));
					ImGui::TableSetupColumn(_("File"));
					ImGui::TableHeadersRow();

					if (ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs()) {
						if (sortSpecs->SpecsDirty && sortSpecs->SpecsCount > 0) {
							const ImGuiTableColumnSortSpecs &sortSpec = sortSpecs->Specs[0];
							const bool asc = sortSpec.SortDirection == ImGuiSortDirection_Ascending;
							switch (sortSpec.ColumnIndex) {
							case 0:
								_scripts.sort([asc](const ScriptInfo &a, const ScriptInfo &b) {
									return asc ? a.name < b.name : a.name > b.name;
								});
								break;
							case 1:
								_scripts.sort([asc](const ScriptInfo &a, const ScriptInfo &b) {
									return asc ? a.type < b.type : a.type > b.type;
								});
								break;
							case 2:
								_scripts.sort([asc](const ScriptInfo &a, const ScriptInfo &b) {
									return asc ? a.author < b.author : a.author > b.author;
								});
								break;
							case 3:
								_scripts.sort([asc](const ScriptInfo &a, const ScriptInfo &b) {
									return asc ? a.version < b.version : a.version > b.version;
								});
								break;
							case 4:
								_scripts.sort([this, asc](const ScriptInfo &a, const ScriptInfo &b) {
									const bool ia = isInstalled(a);
									const bool ib = isInstalled(b);
									return asc ? ia < ib : ia > ib;
								});
								break;
							case 5:
								_scripts.sort([asc](const ScriptInfo &a, const ScriptInfo &b) {
									return asc ? a.filename < b.filename : a.filename > b.filename;
								});
								break;
							default:
								break;
							}
							sortSpecs->SpecsDirty = false;
						}
					}

					for (const voxelui::ScriptInfo &info : _scripts) {
						if (filtered(info)) {
							continue;
						}
						const bool installed = isInstalled(info);
						if (installed) {
							ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
						}
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(info.name.c_str());
						if (!info.description.empty()) {
							ImGui::TooltipTextUnformatted(info.description.c_str());
						}
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(info.type.c_str());
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(info.author.c_str());
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(info.version.c_str());
						ImGui::TableNextColumn();
						if (installed) {
							ImGui::TextUnformatted(ICON_LC_CHECK);
						}
						if (installed) {
							ImGui::PopStyleColor();
						}
						ImGui::TableNextColumn();
						if (ImGui::IconButton(ICON_LC_DOWNLOAD, info.filename.c_str())) {
							voxelui::ScriptApi api;
							if (api.download(_app->filesystem(), SCRIPT_API_URL, info)) {
								Log::info("Downloaded script %s", info.name.c_str());
								_needsReload = true;
							}
						}
						ImGui::TooltipText(_("Download %s"), info.name.c_str());
					}
					ImGui::EndTable();
				}
			}
		}
	}
	ImGui::End();
}

} // namespace voxelui
