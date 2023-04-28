#pragma once

#include <iostream>
#include <string>
#include <vector>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "StringHelper.h"

#undef GetCurrentDirectory
#undef CreateDirectory

class Directory
{
public:
	static std::string GetCurrentDirectory() { return fs::current_path().u8string(); }

	static bool Exists(const fs::path& path) { return fs::exists(path); }

	// Stupid hack because of Windows.h
	static void MakeDirectory(const std::string& path)
	{
		CreateDirectory(path);
	}

	static void CreateDirectory(const std::string& path)
	{

#ifdef _MSC_VER
		std::string splitChar = "\\";
#else
		std::string splitChar = "/";
#endif

		std::string curPath;
		std::vector<std::string> split = StringHelper::Split(path, splitChar);

		for (std::string s : split)
		{
			curPath += s + splitChar;

			if (!Exists(curPath))
				fs::create_directory(curPath);
		}

		// fs::create_directory(path);
	}

	static std::vector<std::string> ListFiles(std::string dir)
	{
		std::vector<std::string> lst;

		if (Directory::Exists(dir))
		{
			for (auto& p : fs::recursive_directory_iterator(dir))
			{
				if (!p.is_directory())
					lst.push_back(p.path().string());
			}
		}

		return lst;
	}
};
