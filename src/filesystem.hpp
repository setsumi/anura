/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FILESYSTEM_HPP_INCLUDED
#define FILESYSTEM_HPP_INCLUDED

#include <boost/cstdint.hpp>
#include <boost/function.hpp>

#include <map>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include "SDL.h"
#include "SDL_rwops.h"
#endif

namespace sys
{

enum FILE_NAME_MODE { ENTIRE_FILE_PATH, FILE_NAME_ONLY };

bool is_directory(const std::string& dname);

//! Populates 'files' with all the files and
//! 'dirs' with all the directories in dir.
//! If files or dirs are NULL they will not be used.
//!
//! Mode determines whether the entire path or just the filename is retrieved.
void get_files_in_dir(const std::string& dir,
                      std::vector<std::string>* files,
                      std::vector<std::string>* dirs=NULL);

//Function which given a directory, will recurse through all sub-directories,
//and find each distinct filename. It will fill the files map such that the
//keys are filenames and the values are the full path to the file.
void get_unique_filenames_under_dir(const std::string& dir,
                                    std::map<std::string, std::string>* file_map,
									const std::string& prefix);

//creates a dir if it doesn't exist and returns the path
std::string get_dir(const std::string& dir);
std::string get_user_data_dir();
std::string get_saves_dir();

std::string read_file(const std::string& fname);
void write_file(const std::string& fname, const std::string& data);

bool dir_exists(const std::string& fname);
bool file_exists(const std::string& fname);
std::string find_file(const std::string& name);

int64_t file_mod_time(const std::string& fname);

#if defined(__ANDROID__)
SDL_RWops* read_sdl_rw_from_asset(const std::string& name);
void print_assets();
#endif // ANDROID

void move_file(const std::string& from, const std::string& to);
void remove_file(const std::string& fname);
void copy_file(const std::string& from, const std::string& to);

void rmdir_recursive(const std::string& path);

bool is_path_absolute(const std::string& path);
std::string make_conformal_path(const std::string& path);
std::string compute_relative_path(const std::string& source, const std::string& target);

struct filesystem_manager {
	filesystem_manager();
	~filesystem_manager();
};

void notify_on_file_modification(const std::string& path, boost::function<void()> handler);
void pump_file_modifications();

}

#endif

