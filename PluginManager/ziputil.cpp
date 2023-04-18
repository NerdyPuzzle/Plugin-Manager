#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#pragma comment(lib, "Urlmon.lib")
#include "ziputil.h"
#include "unzip.h"
#include <vector>
#include <string>
#include <fstream>
#include <atlstr.h>
#include <cstdio>
#include <iostream>
#include <windows.h>
#include <filesystem>
#include <codecvt>

std::wstring s2ws2(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}
std::string ws2s2(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

std::vector<std::string> GetPluginFile(const std::string zip) {
	std::vector<std::string> file;
	TCHAR zip_loc[264];
	_tcscpy_s(zip_loc, CA2T(zip.c_str()));
	HZIP hz = OpenZip(zip_loc, (const char*)0);
	ZIPENTRY ze; GetZipItem(hz, -1, &ze); int numitems = ze.index;
	for (int i = 0; i < numitems; i++)
	{
		GetZipItem(hz, i, &ze);
		int indx = -1;
		FindZipItem(hz, TEXT("plugin.json"), false, &indx, &ze);
		if (indx != -1) {
			UnzipItem(hz, indx, TEXT("temp.txt"));
			std::string line;
			std::ifstream in("temp.txt");
			while (std::getline(in, line))
				file.push_back(line);
			in.close();
			break;
		}
	}
	CloseZip(hz);

	return file;
}

bool DownloadFile(const std::wstring link, const std::wstring location) {
	// URL of the music file, image file etc.
	const wchar_t* sURL = link.c_str();
	// Name of the Destination file 
	const wchar_t* dFile = location.c_str();
	if (S_OK == URLDownloadToFile(NULL, sURL, dFile, 0, NULL))
	{
		std::wcout << L"successfully downloaded file " + link + L"\n";
		return true;
	}
	else
	{
		std::wcerr << L"Failed to download file from link " + link + L"\n";
		return false;
	}
}

void GetUpdateData(bool& shouldupdate) {
	//cache the old links for comparison if present
	std::vector<std::string> oldlinks;
	std::ifstream in1("data/plugindata.txt");
	if (in1.is_open()) {
		while (!in1.eof()) {
			std::string temp;
			in1 >> temp;
			oldlinks.push_back(temp);
		}
	}
	in1.close();

	//download the new links
	TCHAR zip_loc[264] = TEXT("data.zip");
	HZIP hz = OpenZip(zip_loc, (const char*)0);
	ZIPENTRY ze; GetZipItem(hz, -1, &ze); int numitems = ze.index;
	int indx = -1;
	FindZipItem(hz, TEXT("PluginStorage-main/plugindata.txt"), false, &indx, &ze);
	UnzipItem(hz, indx, TEXT("data/plugindata.txt"));
	FindZipItem(hz, TEXT("PluginStorage-main/icons.zip"), false, &indx, &ze);
	UnzipItem(hz, indx, TEXT("data/icons.zip"));
	CloseZip(hz);

	//read new links
	std::vector<std::string> newlinks;
	std::ifstream in2("data/plugindata.txt");
	while (!in2.eof()) {
		std::string temp;
		in2 >> temp;
		newlinks.push_back(temp);
	}

	//compare
	if (oldlinks.empty())
		shouldupdate = true; //no links were present, download all plugins
	else {
		if (oldlinks.size() != newlinks.size()) {
			shouldupdate = true;
			return; //link list isn't the same size, update
		}
		else {
			for (int i = 0; i < newlinks.size(); i++) {
				if (oldlinks[i] != newlinks[i]) {
					shouldupdate = true;
					return; //found a difference, update
				}
			}
		}
	}
}

void freeze(float time) {
	Sleep(time);
}

void UpdateIcons() {
	TCHAR zip_loc[264] = TEXT("data/icons.zip");
	HZIP hz = OpenZip(zip_loc, (const char*)0);
	ZIPENTRY ze; GetZipItem(hz, -1, &ze); int numitems = ze.index;
	for (int i = 0; i < numitems; i++)
	{
		GetZipItem(hz, i, &ze);
		std::string pth = "icons/" + ws2s2(ze.name);
		TCHAR loc[264];
		_tcscpy_s(loc, CA2T(pth.c_str()));
		UnzipItem(hz, i, loc);
	}
	CloseZip(hz);
}