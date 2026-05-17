/**
 * @file
 */

#include "VoxBoxBrowserPanel.h"
#include "ScopedID.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "core/Var.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"

namespace voxedit {

static constexpr const char *CFG_VOXBOX_USERNAME = "ve_voxboxusername";
static constexpr const char *CFG_VOXBOX_PASSWORD = "ve_voxboxpassword";
static constexpr const char *CFG_VOXBOX_APIKEY = "ve_voxboxapikey";

static constexpr int PageCount = 20;

VoxBoxBrowserPanel::VoxBoxBrowserPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
									   const video::TexturePoolPtr &texturePool)
	: Super(app, "voxboxbrowser"), _texturePool(texturePool), _sceneMgr(sceneMgr) {
	_imageQueue = core::make_shared<core::ConcurrentQueue<image::ImagePtr>>();
	_archive = io::openFilesystemArchive(app->filesystem());
	_searchParams.count = PageCount;

	const core::VarDef voxBoxUsername(CFG_VOXBOX_USERNAME, "", N_("VoxBox username"),
									  N_("Your VoxBox account username"));
	_varUsername = core::Var::registerVar(voxBoxUsername);
	const core::VarDef voxBoxPassword(CFG_VOXBOX_PASSWORD, "", N_("VoxBox password"),
									  N_("Your VoxBox account password"), core::CV_SECRET);
	_varPassword = core::Var::registerVar(voxBoxPassword);
	const core::VarDef voxBoxApiKey(CFG_VOXBOX_APIKEY, "", N_("VoxBox API key"),
									N_("VoxBox refresh token for authenticated requests"), core::CV_SECRET);
	_varApiKey = core::Var::registerVar(voxBoxApiKey);

	const core::String &savedToken = _varApiKey->strVal();
	if (!savedToken.empty()) {
		_api.setRefreshToken(savedToken);
	}
}

void VoxBoxBrowserPanel::open() {
	_open = true;
	_requestFocus = true;
	if (_state.info.empty() && !_requestPending) {
		fetchModels();
	}
}

void VoxBoxBrowserPanel::fetchModels() {
	_requestPending = true;
	_state = _api.search(_searchParams);
	_requestPending = false;
}

void VoxBoxBrowserPanel::loginPanel() {
	if (_api.isLoggedIn()) {
		ImGui::Text(_("Logged in as %s"), _api.loggedInUsername().c_str());
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_LOG_OUT, _("Logout"))) {
			_api.logout();
			_varApiKey->setVal("");
		}
		return;
	}

	if (_useApiKey) {
		core::String apiKey = _varApiKey->strVal();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
		if (ImGui::InputTextWithHint("##voxbox_apikey", _("API key (refresh token)"), &apiKey, ImGuiInputTextFlags_Password)) {
			_varApiKey->setVal(apiKey);
		}
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_LOG_IN, _("Connect"))) {
			if (apiKey.empty()) {
				_loginError = _("API key is empty");
			} else {
				_api.setRefreshToken(apiKey);
				_varApiKey->setVal(apiKey);
				_loginError = "";
			}
		}
		ImGui::SameLine();
		if (ImGui::SmallButton(_("Use password"))) {
			_useApiKey = false;
		}
	} else {
		core::String username = _varUsername->strVal();
		core::String password = _varPassword->strVal();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
		if (ImGui::InputTextWithHint("##voxbox_user", _("Username"), &username)) {
			_varUsername->setVal(username);
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
		if (ImGui::InputTextWithHint("##voxbox_pass", _("Password"), &password, ImGuiInputTextFlags_Password)) {
			_varPassword->setVal(password);
		}
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_LOG_IN, _("Login"))) {
			if (_api.login(username, password)) {
				_loginError = "";
				_varPassword->setVal("");
				_varApiKey->setVal(_api.refreshToken());
			} else {
				_loginError = _("Login failed");
			}
		}
		ImGui::SameLine();
		if (ImGui::SmallButton(_("Use API key"))) {
			_useApiKey = true;
		}
	}
	if (!_loginError.empty()) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", _loginError.c_str());
	}
}

void VoxBoxBrowserPanel::searchPanel(command::CommandExecutionListener *listener) {
	if (_requestPending) {
		ImGui::Spinner("fetching_voxbox", ImGui::Size(1.0f));
		ImGui::SameLine();
		ImGui::TextUnformatted(_("Fetching models..."));
		return;
	}

	if (ImGui::IconButton(ICON_LC_SEARCH, _("Search"))) {
		_searchParams.page = 0;
		fetchModels();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	if (ImGui::InputTextWithHint("##voxboxfilter", _("Search by name..."), &_searchParams.nameFilter,
								 ImGuiInputTextFlags_EnterReturnsTrue)) {
		_searchParams.page = 0;
		fetchModels();
	}

	if (ImGui::CollapsingHeader(_("Search Options"))) {
		bool changed = false;

		ImGui::InputTextWithHint(_("Author"), _("Filter by author..."), &_searchParams.authorFilter);

		const char *catLabel =
			_searchParams.category == VoxBoxCategory::Max ? _("All") : VoxBoxCategoryStr(_searchParams.category);
		if (ImGui::BeginCombo(_("Category"), catLabel)) {
			if (ImGui::Selectable(_("All"), _searchParams.category == VoxBoxCategory::Max)) {
				_searchParams.category = VoxBoxCategory::Max;
				changed = true;
			}
			for (int i = 0; i < (int)VoxBoxCategory::Max; ++i) {
				if (ImGui::Selectable(VoxBoxCategoryStr((VoxBoxCategory)i),
									  _searchParams.category == (VoxBoxCategory)i)) {
					_searchParams.category = (VoxBoxCategory)i;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}

		const char *licLabel =
			_searchParams.license == VoxBoxLicense::Max ? _("All") : VoxBoxLicenseStr(_searchParams.license);
		if (ImGui::BeginCombo(_("License"), licLabel)) {
			if (ImGui::Selectable(_("All"), _searchParams.license == VoxBoxLicense::Max)) {
				_searchParams.license = VoxBoxLicense::Max;
				changed = true;
			}
			for (int i = 0; i < (int)VoxBoxLicense::Max; ++i) {
				if (ImGui::Selectable(VoxBoxLicenseStr((VoxBoxLicense)i), _searchParams.license == (VoxBoxLicense)i)) {
					_searchParams.license = (VoxBoxLicense)i;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Checkbox(_("Filter animated"), &_searchParams.useAnimatedFilter)) {
			changed = true;
		}
		if (_searchParams.useAnimatedFilter) {
			ImGui::SameLine();
			if (ImGui::Checkbox(_("Animated only"), &_searchParams.filterAnimated)) {
				changed = true;
			}
		}

		if (changed) {
			_searchParams.page = 0;
			fetchModels();
		}
	}

	if (_state.info.empty()) {
		ImGui::TextUnformatted(_("No models found"));
		return;
	}

	const float paginationHeight = ImGui::GetFrameHeightWithSpacing();
	const ImVec2 tableSize(0.0f, -paginationHeight);
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
									   ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable;
	if (ImGui::BeginTable("##voxboxbrowser", 7, tableFlags, tableSize)) {
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_DefaultSort);
		ImGui::TableSetupColumn(_("Author"));
		ImGui::TableSetupColumn(_("Category"));
		ImGui::TableSetupColumn(_("License"));
		ImGui::TableSetupColumn(_("Rating"));
		ImGui::TableSetupColumn(_("Size"));
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();

		if (ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs()) {
			if (sortSpecs->SpecsDirty && sortSpecs->SpecsCount > 0) {
				const ImGuiTableColumnSortSpecs &spec = sortSpecs->Specs[0];
				const bool asc = spec.SortDirection == ImGuiSortDirection_Ascending;
				switch (spec.ColumnIndex) {
				case 0:
					_state.info.sort([asc](const VoxBoxModelInfo &a, const VoxBoxModelInfo &b) {
						return asc ? a.name < b.name : a.name > b.name;
					});
					break;
				case 1:
					_state.info.sort([asc](const VoxBoxModelInfo &a, const VoxBoxModelInfo &b) {
						return asc ? a.username < b.username : a.username > b.username;
					});
					break;
				case 2:
					_state.info.sort([asc](const VoxBoxModelInfo &a, const VoxBoxModelInfo &b) {
						return asc ? a.category < b.category : a.category > b.category;
					});
					break;
				case 3:
					_state.info.sort([asc](const VoxBoxModelInfo &a, const VoxBoxModelInfo &b) {
						return asc ? a.license < b.license : a.license > b.license;
					});
					break;
				case 4:
					_state.info.sort([asc](const VoxBoxModelInfo &a, const VoxBoxModelInfo &b) {
						return asc ? a.rating < b.rating : a.rating > b.rating;
					});
					break;
				case 5:
					_state.info.sort([asc](const VoxBoxModelInfo &a, const VoxBoxModelInfo &b) {
						return asc ? a.size < b.size : a.size > b.size;
					});
					break;
				default:
					break;
				}
				sortSpecs->SpecsDirty = false;
			}
		}

		for (const VoxBoxModelInfo &info : _state.info) {
			const bool downloaded = VoxBoxApi::isDownloaded(_app->filesystem(), info);
			if (downloaded) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
			}
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(info.name.c_str());
			if (!info.description.empty()) {
				ImGui::TooltipTextUnformatted(info.description.c_str());
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(info.username.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(VoxBoxCategoryStr(info.category));
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(VoxBoxLicenseStr(info.license));
			ImGui::TableNextColumn();
			if (info.totalRatings > 0) {
				ImGui::Text("%.1f (%d)", info.rating, info.totalRatings);
			} else {
				ImGui::TextUnformatted("-");
			}
			ImGui::TableNextColumn();
			if (info.size > 1024 * 1024) {
				ImGui::Text("%.1f MB", (float)info.size / (1024.0f * 1024.0f));
			} else if (info.size > 1024) {
				ImGui::Text("%.1f KB", (float)info.size / 1024.0f);
			} else {
				ImGui::Text("%d B", (int)info.size);
			}
			if (downloaded) {
				ImGui::PopStyleColor();
			}
			ImGui::TableNextColumn();
			ImGui::PushID(info.id.c_str());
			if (downloaded) {
				if (ImGui::IconButton(ICON_LC_FILE_INPUT, "##load")) {
					const core::String fullPath = _app->filesystem()->homeWritePath(VoxBoxApi::vengiPath(info));
					io::FileDescription fd;
					fd.set(fullPath);
					_sceneMgr->load(fd);
				}
				ImGui::TooltipTextUnformatted(_("Load as new scene"));
				ImGui::SameLine();
				if (ImGui::IconButton(ICON_LC_SQUARE_PLUS, "##import")) {
					const core::String fullPath = _app->filesystem()->homeWritePath(VoxBoxApi::vengiPath(info));
					_sceneMgr->import(fullPath);
				}
				ImGui::TooltipTextUnformatted(_("Import into current scene"));
				ImGui::SameLine();
				if (ImGui::IconButton(ICON_LC_TRASH, "##remove")) {
					VoxBoxApi::removeDownload(_app->filesystem(), info);
				}
				ImGui::TooltipTextUnformatted(_("Remove downloaded file"));
			} else {
				if (ImGui::IconButton(ICON_LC_DOWNLOAD, "##download")) {
					const core::String vengiPath = _api.download(_app->filesystem(), info);
					if (!vengiPath.empty()) {
						io::FileDescription fd;
						fd.set(_app->filesystem()->homeWritePath(vengiPath));
						_sceneMgr->load(fd);
					}
				}
				ImGui::TooltipTextUnformatted(_("Download and open"));
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	int pages = (_state.count + PageCount - 1) / PageCount;
	for (int i = 0; i < pages; ++i) {
		if (i == _searchParams.page) {
			ImGui::Text("[%d]", i + 1);
		} else {
			const core::String pageId = core::String::format("%d##voxbox_page", i + 1);
			if (ImGui::Button(pageId.c_str())) {
				_searchParams.page = i;
				fetchModels();
			}
		}
		if (i < pages - 1) {
			ImGui::SameLine();
		}
	}
}

void VoxBoxBrowserPanel::uploadPanel() {
	if (!_api.isLoggedIn()) {
		ImGui::TextUnformatted(_("Please log in to upload models"));
		return;
	}

	ui::ScopedID id("upload");

	ImGui::InputText(_("Name"), &_uploadInfo.name);
	ImGui::InputTextMultiline(_("Description"), &_uploadInfo.description, ImVec2(0, ImGui::GetTextLineHeight() * 3));

	if (ImGui::BeginCombo(_("Category"), VoxBoxCategoryStr(_uploadInfo.category))) {
		for (int i = 0; i < (int)VoxBoxCategory::Max; ++i) {
			if (ImGui::Selectable(VoxBoxCategoryStr((VoxBoxCategory)i), _uploadInfo.category == (VoxBoxCategory)i)) {
				_uploadInfo.category = (VoxBoxCategory)i;
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo(_("License"), VoxBoxLicenseStr(_uploadInfo.license))) {
		for (int i = 0; i < (int)VoxBoxLicense::Max; ++i) {
			if (ImGui::Selectable(VoxBoxLicenseStr((VoxBoxLicense)i), _uploadInfo.license == (VoxBoxLicense)i)) {
				_uploadInfo.license = (VoxBoxLicense)i;
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Checkbox(_("Animated"), &_uploadInfo.animated);
	ImGui::SameLine();
	ImGui::Checkbox(_("Public"), &_uploadInfo.isPublic);

	if (!_uploadInfo.id.empty()) {
		ImGui::Text(_("Updating existing model: %s"), _uploadInfo.id.c_str());
	}

	// TODO: generate a preview image from the scene if no cover is selected using the thumbnail renderer
	static const io::FormatDescription pngFormat[]{io::format::png(), io::FormatDescription::END};
	ImGui::InputFile(_("Cover image"), true, &_uploadCoverFile, pngFormat);

	const bool canUpload = !_uploadInfo.name.empty() && !_uploadCoverFile.empty();
	if (ImGui::DisabledIconButton(ICON_LC_UPLOAD, _("Submit"), !canUpload)) {
		scenegraph::SceneGraph &sg = _sceneMgr->sceneGraph();
		const core::String voxPath = VoxBoxApi::exportToVox(_app->filesystem(), sg);
		if (!voxPath.empty()) {
			if (_api.upload(_app->filesystem(), voxPath, _uploadCoverFile, _uploadInfo)) {
				VoxBoxApi::writeMetadata(sg, _uploadInfo);
				_showUpload = false;
			}
			io::Filesystem::sysRemoveFile(voxPath);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button(_("Cancel"))) {
		_showUpload = false;
	}
}

void VoxBoxBrowserPanel::update(const char *id, command::CommandExecutionListener *listener) {
	if (!_open) {
		return;
	}

	const core::String title = makeTitle(ICON_LC_CLOUD_DOWNLOAD, _("VoxBox Browser"), id);
	if (_requestFocus) {
		ImGui::SetNextWindowFocus();
		_requestFocus = false;
	}
	if (ImGui::Begin(title.c_str(), &_open)) {
		loginPanel();
		ImGui::SameLine();
		ImGui::URLIconButton(ICON_LC_EXTERNAL_LINK, _("VoxBox Store"), "https://voxbox.store");
		ImGui::Separator();

		if (_showUpload && _api.isLoggedIn()) {
			uploadPanel();
			ImGui::Separator();
		} else {
			if (_api.isLoggedIn()) {
				const bool sceneEmpty = _sceneMgr->sceneGraph().empty();
				const VoxBoxModelInfo sceneInfo = VoxBoxApi::readMetadata(_sceneMgr->sceneGraph());
				const core::String loggedInUserId = _api.loggedInUserId();
				const bool isUpdate = !sceneInfo.id.empty() && sceneInfo.userId == loggedInUserId;
				const bool isNew = sceneInfo.id.empty();
				if (isNew) {
					if (ImGui::DisabledIconButton(ICON_LC_CLOUD_UPLOAD, _("Upload current scene..."), sceneEmpty)) {
						_uploadInfo = sceneInfo;
						_showUpload = true;
					}
					ImGui::SameLine();
				} else if (isUpdate) {
					if (ImGui::DisabledIconButton(ICON_LC_CLOUD_UPLOAD, _("Update on VoxBox..."), sceneEmpty)) {
						_uploadInfo = sceneInfo;
						_showUpload = true;
					}
					ImGui::SameLine();
				}
			}
			searchPanel(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
