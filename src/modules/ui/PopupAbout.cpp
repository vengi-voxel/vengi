/**
 * @file
 */

#include "PopupAbout.h"
#include "app/App.h"
#include "engine-config.h"
#include "engine-git.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/Panel.h"

namespace ui {

void metricOption() {
	const core::VarPtr &metricFlavor = core::Var::getSafe(cfg::MetricFlavor);
	bool metrics = !metricFlavor->strVal().empty();
	if (ImGui::IconCheckbox(ICON_LC_CHART_AREA, _("Enable sending anonymous metrics"), &metrics)) {
		if (metrics) {
			metricFlavor->setVal("json");
		} else {
			metricFlavor->setVal("");
		}
	}
	ImGui::TooltipTextUnformatted(_("Send anonymous usage statistics"));
}

void popupAbout(const std::function<void()> &customTabs, bool isNewVersionAvailable) {
	int popupWidth = ImGui::Size(60);
	int popupHeight = ImGui::Height(20);
	ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight), ImGuiCond_Appearing);
	const core::String title = Panel::makeTitle(_("About"), POPUP_TITLE_ABOUT);
	if (ImGui::BeginPopupModal(title.c_str())) {
		popupWidth = ImGui::GetWindowWidth();
		popupHeight = ImGui::GetWindowHeight();
		const float footerHeight = ImGui::GetStyle().ItemSpacing.y * 3 + ImGui::GetFrameHeightWithSpacing() * 2;
		if (ImGui::BeginChild("##scrollwindow", ImVec2(popupWidth, popupHeight - footerHeight))) {
			if (ImGui::BeginTabBar("##abouttabbar")) {
				const float urlIconWidth = ImGui::GetContentRegionAvail().x;

				if (ImGui::BeginTabItem(app::App::getInstance()->fullAppname().c_str())) {
					ImGui::Text("%s " PROJECT_VERSION, app::App::getInstance()->appname().c_str());
					ImGui::BulletText(GIT_COMMIT " - " GIT_COMMIT_DATE);

					ImGui::Dummy(ImVec2(1, 10));
					ImGui::TextUnformatted(_("This is a beta release!"));
					if (isNewVersionAvailable) {
						ImGui::TextUnformatted(_("A new version is available!"));
					} else {
						ImGui::TextUnformatted(_("You are using the latest version."));
					}
#ifndef NDEBUG
					ImGui::TextUnformatted(_("Debug build with reduced performance"));
#endif
					metricOption();

					ImGui::Dummy(ImVec2(1, 10));
					ImGui::URLIconItem(ICON_LC_CIRCLE_QUESTION_MARK, _("Website"), "https://vengi-voxel.github.io/vengi/",
									   urlIconWidth);
					ImGui::URLIconItem(ICON_LC_GITHUB, _("Bug reports"), "https://github.com/vengi-voxel/vengi/issues",
									   urlIconWidth);
					ImGui::URLIconItem(ICON_LC_SQUARE, _("Mastodon"), "https://mastodon.social/@mgerhardy",
									   urlIconWidth);
					ImGui::URLIconItem(ICON_LC_SQUARE, _("Discord"), "https://vengi-voxel.de/discord", urlIconWidth);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(_("Credits"))) {
					ImGui::URLItem("backward-cpp", "https://github.com/bombela/backward-cpp", urlIconWidth);
#ifdef USE_CURL
					ImGui::Text("libCURL");
#endif
					ImGui::URLItem("cute_aseprite", "https://github.com/RandyGaul/cute_headers", urlIconWidth);
					ImGui::URLItem("dearimgui", "https://github.com/ocornut/imgui", urlIconWidth);
					ImGui::URLItem("glm", "https://github.com/g-truc/glm", urlIconWidth);
					ImGui::URLItem("IconFontCppHeaders", "https://github.com/juliettef/IconFontCppHeaders",
								   urlIconWidth);
					ImGui::URLItem("imguizmo", "https://github.com/CedricGuillemet/ImGuizmo", urlIconWidth);
					ImGui::URLItem("im-neo-sequencer", "https://gitlab.com/GroGy/im-neo-sequencer", urlIconWidth);
					ImGui::URLItem("implot", "https://github.com/epezent/implot", urlIconWidth);
					ImGui::URLItem("libvxl", "https://github.com/xtreme8000/libvxl", urlIconWidth);
#ifdef USE_LIBJPEG
					ImGui::URLItem("libjpeg", "https://github.com/libjpeg-turbo/libjpeg-turbo", urlIconWidth);
#endif
					ImGui::URLItem("lua", "https://www.lua.org/", urlIconWidth);
					ImGui::URLItem("lucide", "https://lucide.dev/", urlIconWidth);
					ImGui::URLItem("meshoptimizer", "https://github.com/zeux/meshoptimizer", urlIconWidth);
					ImGui::URLItem("ogt_vox", "https://github.com/jpaver/opengametools", urlIconWidth);
					ImGui::URLItem("polyvox", "http://www.volumesoffun.com/", urlIconWidth);
					ImGui::URLItem("SDL2", "https://github.com/libsdl-org/SDL", urlIconWidth);
					ImGui::URLItem("stb/SOIL2", "https://github.com/SpartanJ/SOIL2", urlIconWidth);
					ImGui::URLItem("tinygettext", "https://github.com/tinygettext/tinygettext/", urlIconWidth);
					ImGui::URLItem("tinygltf", "https://github.com/syoyo/tinygltf", urlIconWidth);
					ImGui::URLItem("tinyobjloader", "https://github.com/tinyobjloader/tinyobjloader", urlIconWidth);
					ImGui::URLItem("ufbx", "https://github.com/bqqbarbhg/ufbx", urlIconWidth);
					ImGui::URLItem("Yocto/GL", "https://github.com/xelatihy/yocto-gl", urlIconWidth);
#if defined(USE_DEFLATE)
					ImGui::Text("libdeflate");
#elif defined(USE_ZLIB)
					ImGui::Text("zlib");
#else
					ImGui::Text("miniz");
#endif
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(_("Paths"))) {
					for (const core::String &path : io::filesystem()->registeredPaths()) {
						const core::String &abspath = io::filesystem()->sysAbsolutePath(path);
						if (abspath.empty()) {
							continue;
						}
						core::String fileurl = "file://" + abspath;
						ImGui::Bullet();
						ImGui::SameLine();
						ImGui::URLItem(abspath.c_str(), fileurl.c_str(), urlIconWidth);
					}
					ImGui::EndTabItem();
				}
				if (customTabs) {
					customTabs();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();
		if (ImGui::IconButton(ICON_LC_X, _("Close"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

} // namespace ui
