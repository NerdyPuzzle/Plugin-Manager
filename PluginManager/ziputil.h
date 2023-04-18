#pragma once
#include <vector>
#include <string>

std::vector<std::string> GetPluginFile(const std::string zip);
bool DownloadFile(const std::wstring link, const std::wstring location);
void GetUpdateData(bool& shouldupdate);
void freeze(float time);
void UpdateIcons();
