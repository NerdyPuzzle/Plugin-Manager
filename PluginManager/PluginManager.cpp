#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define DARK_STYLE
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include <raylib.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iostream>
#include "rlImGui.h"
#include "ziputil.h"
#include <filesystem>
#include <sstream>
#include <map>
#include <fstream>
#include <codecvt>
#include "styles.h"

namespace fs = std::filesystem;

int indexOf(const std::vector<std::string> data_, const std::string& element) {
    auto it = std::find(data_.begin(), data_.end(), element);
    return (it == data_.end() ? -1 : std::distance(data_.begin(), it));
}

int current_plugin = -1;
int old_current_plugin = -1;
bool delete_mode = false;
bool delete_set_pos = true;

std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}
std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

std::string plugindir = (std::string)getenv("USERPROFILE") + "\\.mcreator\\plugins\\";
std::string workspacedir = (std::string)getenv("USERPROFILE") + "\\MCreatorWorkspaces\\";
std::vector<std::vector<std::string>> plugins;
std::vector<std::string> installed_plugin_paths;
std::vector<std::vector<std::string>> available_plugins;
std::vector<std::string> available_plugin_paths;
std::vector<std::string> updated_plugins;
std::vector<std::string> updated_plugin_paths;
std::map<std::string, std::vector<std::string>> plugin_info;
std::map<std::string, Texture> plugin_icons;
std::string MCreatorVersion;
int workspaces = 0;

int GetWorkspaceDirSize() {
    int size = 0;
    if (DirectoryExists(workspacedir.c_str()))
        for (const fs::path entry : fs::directory_iterator(workspacedir))
            if (fs::is_directory(entry) && !entry.empty())
                size++;
    return size;
}


std::string GetPluginName(std::vector<std::string> plugin) {
    std::string name = "Unknown";
    bool hasname = false;
    for (const std::string line : plugin) {
        std::istringstream iss(line);
        std::string temp = "";
        while (iss >> temp) {
            if (temp.find("\"name\"") != std::string::npos) {
                std::string newname = "";
                while (iss >> temp) {
                    if (newname != "")
                        newname += " ";
                    newname += temp;
                }
                temp = newname;
                hasname = true;
                break;
            }
        }
        if (hasname) {
            name.clear();
            for (int i = 0; i < temp.size(); i++) {
                if (temp[i] == ',' && i > 0 && temp[i - 1] == '\"')
                    break;
                if (temp[i] != '\"')
                    name += temp[i];
            }
            break;
        }
    }
    return name;
}
std::string IsJavaPlugin(std::vector<std::string> plugin) {
    for (const std::string line : plugin)
        if (line.find("\"javaplugin\"") != std::string::npos)
            return "yes";
    return "no";
}
std::string GetPluginVersion(std::vector<std::string> plugin) {
    std::string version = "Unknown";
    int numcount = 0;
    bool versionmode = false;
    for (const std::string line : plugin) {
        if (line.find("\"version\"") != std::string::npos) {
            version.clear();
            for (int i = 0; i < line.size(); i++) {
                if (line[i] == '"')
                    numcount++;
                if (numcount == 4)
                    break;
                if (numcount == 3) {
                    if (versionmode)
                        version += line[i];
                    else
                        versionmode = true;
                }
            }
        }
    }
    return version;
}
bool IsVersionGreater(std::vector<std::string> plugin1, std::vector<std::string> plugin2) {
    std::string version1 = GetPluginVersion(plugin1);
    if (version1 == "Unknown")
        return false;
    std::string temp1 = "";
    for (int i = 0; i < version1.size(); i++) {
        if (std::isdigit(version1[i]))
            temp1 += version1[i];
    }
    version1 = temp1;
    std::string version2 = GetPluginVersion(plugin2);
    if (version2 == "Unknown")
        return true;
    std::string temp2 = "";
    for (int i = 0; i < version2.size(); i++) {
        if (std::isdigit(version2[i]))
            temp2 += version2[i];
    }
    version2 = temp2;
    if ((std::stoi(version1) > std::stoi(version2)) || (std::stoi(std::to_string(version1[0])) > std::stoi(std::to_string(version2[0]))) || (std::stoi(std::to_string(version1[0] + version1[1])) > (std::stoi(std::to_string(version2[0] + version2[1])))))
        return true;
    return false;
}
std::string GetMinVersion(std::vector<std::string> plugin) {
    std::string version = "Unknown";
    int numcount = 0;
    bool versionmode = false;
    for (const std::string line : plugin) {
        if (line.find("\"minversion\"") != std::string::npos) {
            version.clear();
            for (int i = 0; i < line.size(); i++) {
                if (line[i] == '"')
                    numcount++;
                if (line[i] == ',')
                    break;
                if (numcount == 2) {
                    if (versionmode && line[i] != ' ' && line[i] != ':')
                        version += line[i];
                    else
                        versionmode = true;
                }
            }
            if (version == "")
                version = "Unknown";
        }
    }
    return version;
}
std::string GetMaxVersion(std::vector<std::string> plugin) {
    std::string version = "Unknown";
    int numcount = 0;
    bool versionmode = false;
    for (const std::string line : plugin) {
        if (line.find("\"maxversion\"") != std::string::npos) {
            version.clear();
            for (int i = 0; i < line.size(); i++) {
                if (line[i] == '"')
                    numcount++;
                if (line[i] == ',')
                    break;
                if (numcount == 2) {
                    if (versionmode && line[i] != ' ' && line[i] != ':')
                        version += line[i];
                    else
                        versionmode = true;
                }
            }
            if (version == "")
                version = "Unknown";
        }
    }
    return version;
}
std::string GetPluginDescription(std::vector<std::string> plugin) {
    std::string version = "Unknown";
    int numcount = 0;
    bool versionmode = false;
    for (const std::string line : plugin) {
        if (line.find("\"description\"") != std::string::npos) {
            version.clear();
            for (int i = 0; i < line.size(); i++) {
                if (line[i] == '"')
                    numcount++;
                if (numcount == 4)
                    break;
                if (numcount == 3) {
                    if (versionmode)
                        version += line[i];
                    else
                        versionmode = true;
                }
            }
        }
    }
    return version;
}
std::string GetPluginCredits(std::vector<std::string> plugin) {
    std::string version = "Unknown";
    int numcount = 0;
    bool versionmode = false;
    for (const std::string line : plugin) {
        if (line.find("\"credits\"") != std::string::npos) {
            version.clear();
            for (int i = 0; i < line.size(); i++) {
                if (line[i] == '"')
                    numcount++;
                if (numcount == 4)
                    break;
                if (numcount == 3) {
                    if (versionmode)
                        version += line[i];
                    else
                        versionmode = true;
                }
            }
        }
    }
    return version;
}
std::string GetPluginAuthor(std::vector<std::string> plugin) {
    std::string version = "Unknown";
    int numcount = 0;
    bool versionmode = false;
    for (const std::string line : plugin) {
        if (line.find("\"author\"") != std::string::npos) {
            version.clear();
            for (int i = 0; i < line.size(); i++) {
                if (line[i] == '"')
                    numcount++;
                if (numcount == 4)
                    break;
                if (numcount == 3) {
                    if (versionmode)
                        version += line[i];
                    else
                        versionmode = true;
                }
            }
        }
    }
    return version;
}
std::string GetMCreatorVersion() {
    std::string version = "Unknown";
    float vers = 0;
    if (DirectoryExists(workspacedir.c_str())) {
        for (const fs::path entry : fs::directory_iterator(workspacedir)) {
            if (fs::is_directory(entry) && !entry.empty()) {
                workspaces++;
                std::ifstream in(entry.string() + "\\" + entry.filename().string() + ".mcreator");
                if (in.is_open() && in.peek() != std::ifstream::traits_type::eof()) {
                    std::string temp;
                    while (std::getline(in, temp)) {
                        if (temp.find("\"mcreatorVersion\"") != std::string::npos) {
                            version.clear();
                            int numcount = 0;
                            bool versionmode = false;
                            for (int i = 0; i < temp.size(); i++) {
                                if (std::isdigit(temp[i]))
                                    version += temp[i];
                            }
                            break;
                        }
                        std::cout << temp << "\n";
                    }
                }
                in.close();
                std::string ef = version;
                if (version == "")
                    version = "Unknown";
                if (version != "Unknown") {
                    if (std::stof(version) > vers)
                        vers = std::stof(version);
                }
                rlImGuiBegin();
                ImGui::SetNextWindowPos({ 0 , 0 });
                ImGui::SetNextWindowSize({ (float)GetScreenWidth(), (float)GetScreenHeight() });
                if (ImGui::Begin("boot1", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
                    ImGui::SetWindowFontScale(3);
                    ImGui::Text("Looking for MCreator version...");
                    std::string s = "Found version: " + std::to_string((long long)vers);
                    if (vers != 0)
                        ImGui::Text(s.c_str());
                    ImGui::End();
                }
                rlImGuiEnd();
                BeginDrawing();
                EndDrawing();
                version = "Unknown";
            }
        }
    }
    if (vers != 0) {
        long long version_ = (long long)vers;
        version = std::to_string(version_);
    }
    return version;
}

void RefreshPlugins() {
    plugins.clear();
    available_plugins.clear();
    available_plugin_paths.clear();
    updated_plugins.clear();
    updated_plugin_paths.clear();
    plugin_info.clear();
    installed_plugin_paths.clear();

    if (DirectoryExists(plugindir.c_str())) {
        for (const fs::path entry : fs::directory_iterator(plugindir)) {
            std::vector<std::string> plugin = GetPluginFile(entry.string());
            if (!plugin.empty()) {
                plugins.push_back(plugin);
                installed_plugin_paths.push_back(entry.string());
            }
        }
    }

    for (const std::vector<std::string> plugin : plugins) {
        plugin_info["names"].push_back(GetPluginName(plugin));
        plugin_info["javaplugins"].push_back(IsJavaPlugin(plugin));
        plugin_info["installed"].push_back("yes");
        plugin_info["versions"].push_back(GetPluginVersion(plugin));
        plugin_info["minversions"].push_back(GetMinVersion(plugin));
        plugin_info["descriptions"].push_back(GetPluginDescription(plugin));
        plugin_info["authors"].push_back(GetPluginAuthor(plugin));
        plugin_info["credits"].push_back(GetPluginCredits(plugin));
        plugin_info["maxversions"].push_back(GetMaxVersion(plugin));
    }

    if (DirectoryExists("plugins/")) {
        for (const fs::path entry : fs::directory_iterator("plugins/")) {
            std::vector<std::string> plugin = GetPluginFile(entry.string());
            if (!plugin.empty()) {
                std::string name = GetPluginName(plugin);

                if (std::find(plugin_info["names"].begin(), plugin_info["names"].end(), name) == plugin_info["names"].end()) {
                    available_plugins.push_back(plugin);
                    available_plugin_paths.push_back(entry.string());
                    plugins.push_back(plugin);
                }
                else if (std::find(plugin_info["names"].begin(), plugin_info["names"].end(), name) != plugin_info["names"].end() && IsVersionGreater(plugin, plugins[indexOf(plugin_info["names"], name)])) {
                    updated_plugins.push_back(name);
                    updated_plugin_paths.push_back(entry.string());
                }
            }
        }
    }

    for (const std::vector<std::string> plugin : available_plugins) {
        plugin_info["names"].push_back(GetPluginName(plugin));
        plugin_info["javaplugins"].push_back(IsJavaPlugin(plugin));
        plugin_info["installed"].push_back("no");
        plugin_info["versions"].push_back(GetPluginVersion(plugin));
        plugin_info["minversions"].push_back(GetMinVersion(plugin));
        plugin_info["descriptions"].push_back(GetPluginDescription(plugin));
        plugin_info["authors"].push_back(GetPluginAuthor(plugin));
        plugin_info["credits"].push_back(GetPluginCredits(plugin));
        plugin_info["maxversions"].push_back(GetMaxVersion(plugin));
    }
}
void DownloadUpdates() {
    std::wifstream in("data/plugindata.txt");
    std::vector<std::wstring> newlinks;
    while (!in.eof()) {
        std::wstring s;
        in >> s;
        newlinks.push_back(s);
    }
    fs::remove_all("plugins/");
    fs::create_directory("plugins/");
    int counter = 0;
    bool first = true;
    std::vector<std::string> results;
    for (const std::wstring s : newlinks) {
        results.push_back(" status: Downloading...");
        counter++;
        rlImGuiBegin();
        ImGui::SetNextWindowPos({ 0 , 0 });
        ImGui::SetNextWindowSize({ (float)GetScreenWidth(), (float)GetScreenHeight() });
        if (ImGui::Begin("boot", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
            ImGui::SetWindowFontScale(1.2);
            ImGui::BeginChild(1);
            for (int j = 0; j < counter; j++) {
                std::string temps = "Downloading file " + ws2s(newlinks[j]) + results[j];
                ImGui::Text(temps.c_str());
            }
            ImGui::EndChild();
            ImGui::End();
        }
        rlImGuiEnd();
        BeginDrawing();
        EndDrawing();
        if (s != L"") {
            if (DownloadFile(s, L"plugins/plugin" + std::to_wstring(counter) + L".zip"))
                results[counter - 1] = " status: Success.";
            else
                results[counter - 1] = " status: Failed.";
            first = false;
        }
    }
    in.close();
    if (FileExists("data/icons.zip")) {
        fs::remove_all("icons/");
        fs::create_directory("icons/");
        UpdateIcons();
    }
}

void main() {
    InitWindow(1200, 600, "Plugin Manager");
    Image icon = LoadImage("data/mainicon.png");
    SetWindowIcon(icon);
    SetWindowMinSize(600, 300);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    rlImGuiSetup(true);
    SetGuiStyle();

    if (FileExists("data.zip"))
        fs::remove("data.zip");
    if (DownloadFile(L"https://github.com/NerdyPuzzle/PluginStorage/archive/refs/heads/main.zip", L"data.zip")) {
        bool shouldupdate = false;
        GetUpdateData(shouldupdate);
        if (shouldupdate) {
            MCreatorVersion = GetMCreatorVersion();
            std::ofstream out("data/version.txt");
            out << MCreatorVersion;
            out.close();
            DownloadUpdates();
        }
        else {
            std::ifstream in("data/version.txt");
            in >> MCreatorVersion;
            in.close();
        }
    }
    RefreshPlugins();

    for (const std::string plugin : plugin_info["names"]) {
        std::string file = "icons/" + plugin + ".png";
        if (FileExists(file.c_str()))
            plugin_icons[plugin] = LoadTexture(file.c_str());
        else
            plugin_icons[plugin] = LoadTexture("icons/Unknown.png");
    }

    Texture Javaplugin = LoadTexture("icons/javaplugin.png");

    while (!WindowShouldClose()) {

        ClearBackground(BLACK);
        rlImGuiBegin();

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ (float)GetScreenWidth(), (float)GetScreenHeight() });
        if (ImGui::Begin("Test", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
            if (!plugins.empty()) {
                if (ImGui::BeginTabBar("Plugins", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton)) {
                    for (int i = 0; i < plugins.size(); i++) {
                        if (ImGui::BeginTabItem(plugin_info["names"][i].c_str())) {
                            current_plugin = i;
                            if (old_current_plugin != current_plugin)
                                delete_mode = false;
                            old_current_plugin = current_plugin;
                            rlImGuiImageSize(&plugin_icons[plugin_info["names"][i]], 175, 175);
                            ImGui::SameLine();
                            ImGui::BeginChild(1);
                            ImGui::NewLine();
                            ImGui::SetWindowFontScale(3);
                            ImGui::Text(plugin_info["names"][i].c_str());
                            ImGui::SetWindowFontScale(2);
                            ImGui::Text(plugin_info["versions"][i].c_str());
                            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
                            ImGui::SetWindowFontScale(1);
                            if (std::find(updated_plugins.begin(), updated_plugins.end(), plugin_info["names"][i]) != updated_plugins.end()) {
                                if (ImGui::Button("Update", { 120, 30 })) {
                                    int index = -1;
                                    std::string path1 = "";
                                    std::string path2 = "";
                                    for (int j = 0; j < plugin_info["installed"].size(); j++) {
                                        if (plugin_info["installed"][j] == "yes") {
                                            index++;
                                            if (plugin_info["names"][j] == plugin_info["names"][i])
                                                break;
                                        }
                                    }
                                    path1 = installed_plugin_paths[index];
                                    index = -1;
                                    for (int j = 0; j < updated_plugins.size(); j++) {
                                        index++;
                                        if (updated_plugins[j] == plugin_info["names"][i])
                                            break;
                                    }
                                    path2 = updated_plugin_paths[index];
                                    fs::remove(path1);
                                    fs::copy_file(path2, path1);
                                    RefreshPlugins();
                                }
                            }
                            if (plugin_info["installed"][i] == "yes") {
                                if (ImGui::Button("Uninstall", { 120, 30 })) {
                                    delete_mode = true;
                                }
                            }
                            else if (plugin_info["installed"][i] == "no") {
                                if (ImGui::Button("Install", { 120, 30 })) {
                                    int index = -1;
                                    for (int j = 0; j < plugin_info["installed"].size(); j++) {
                                        if (plugin_info["installed"][j] == "no") {
                                            index++;
                                            if (plugin_info["names"][j] == plugin_info["names"][i])
                                                break;
                                        }
                                    }
                                    fs::copy_file(available_plugin_paths[index], plugindir + (std::string)GetFileName(available_plugin_paths[index].c_str()));
                                    RefreshPlugins();
                                }
                            }
                            ImGui::EndChild();
                            ImGui::SetNextWindowPos({ 0, 210 });
                            ImGui::SetNextWindowSize({ (float)GetScreenWidth(), (float)GetScreenHeight() - 200});
                            if (ImGui::Begin(" ", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration)) {
                                ImGui::SetWindowFontScale(1.5);
                                ImGui::Spacing();
                                std::string author = "Author(s): " + plugin_info["authors"][i];
                                std::string minversion = "Minimum MCreator version: " + plugin_info["minversions"][i];
                                std::string maxversion = "Maximum MCreator version: " + plugin_info["maxversions"][i];
                                std::string description = "Description: " + plugin_info["descriptions"][i];
                                std::string credits = "Credits: " + plugin_info["credits"][i];
                                ImGui::Text(author.c_str());
                                ImGui::Text(minversion.c_str());
                                ImGui::Text(maxversion.c_str());
                                ImGui::Text(description.c_str());
                                ImGui::Text(credits.c_str());
                                if (plugin_info["javaplugins"][i] == "yes") {
                                    ImGui::NewLine();
                                    rlImGuiImageSize(&Javaplugin, 50, 50);
                                    ImGui::SameLine();
                                    ImGui::Spacing();
                                    ImGui::SameLine();
                                    ImGui::BeginChild(2);
                                    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
                                    ImGui::Text("Requires java plugins");
                                    ImGui::EndChild();
                                }
                            }
                            ImGui::End();
                            ImGui::EndTabItem();
                        }
                    }
                }
                ImGui::EndTabBar();
            }
            else
                ImGui::Text("No plugins exist or were found. (this is very bad)");
        }
        ImGui::End();

        if (delete_mode) {
            ImGui::OpenPopup("Are you sure?");
            ImGui::SetNextWindowFocus();
            if (delete_set_pos)
                ImGui::SetNextWindowPos({ ((float)GetScreenWidth() - 350) / 2, ((float)GetScreenHeight() - 125) / 2 });
            delete_set_pos = false;
            ImGui::SetNextWindowSize({ 350, 125 });
            if (ImGui::BeginPopupModal("Are you sure?", &delete_mode, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
                ImGui::NewLine();
                ImGui::SetCursorPosX(10);
                ImGui::Text("Are you sure you want to uninstall this plugin?");
                ImGui::NewLine();
                ImGui::Spacing();
                ImGui::SetCursorPosX(85);
                if (ImGui::Button("Yes", { 75, 30 })) {
                    fs::remove(installed_plugin_paths[current_plugin]);
                    RefreshPlugins();
                    delete_mode = false;
                }
                ImGui::SameLine();
                ImGui::SetCursorPosX(185);
                if (ImGui::Button("No", { 75, 30 }))
                    delete_mode = false;
                ImGui::End();
            }
        }
        else
            delete_set_pos = true;

        rlImGuiEnd();
        BeginDrawing();
        EndDrawing();
    }

    for (const std::string plugin : plugin_info["names"])
        UnloadTexture(plugin_icons[plugin]);
    UnloadTexture(Javaplugin);
    UnloadImage(icon);

    rlImGuiShutdown();
    CloseWindow();

}
