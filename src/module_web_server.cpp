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
#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <deque>
#include <iostream>

#if !defined(_WINDOWS)
#include <sys/time.h>
#endif

#include "asserts.hpp"
#include "base64.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "md5.hpp"
#include "module_web_server.hpp"
#include "string_utils.hpp"
#include "utils.hpp"
#include "unit_test.hpp"
#include "variant.hpp"
#include "variant_utils.hpp"

using boost::asio::ip::tcp;

module_web_server::module_web_server(const std::string& data_path, boost::asio::io_service& io_service, int port)
	: http::web_server(io_service, port), timer_(io_service), 
	nheartbeat_(0), data_path_(data_path), next_lock_id_(1)
{
	if(data_path_.empty() || data_path_[data_path_.size()-1] != '/') {
		data_path_ += "/";
	}

	if(sys::file_exists(data_file_path())) {
		data_ = json::parse_from_file(data_file_path());
	} else {
		std::map<variant, variant> m;
		data_ = variant(&m);
	}

	heartbeat();
}

void module_web_server::heartbeat()
{
	timer_.expires_from_now(boost::posix_time::seconds(1));
	timer_.async_wait(boost::bind(&module_web_server::heartbeat, this));
}

void module_web_server::handle_post(socket_ptr socket, variant doc, const http::environment& env)
{
	std::map<variant,variant> response;
	try {
		const std::string msg_type = doc["type"].as_string();
		if(msg_type == "download_module") {
			const std::string module_id = doc["module_id"].as_string();

			if(doc.has_key("current_version")) {
				variant current_version = doc["current_version"];
				if(data_[module_id]["version"] <= current_version) {
					send_msg(socket, "text/json", "{ status: \"no_newer_module\" }", "");
					return;
				}
			}

			const std::string module_path = data_path_ + module_id + ".cfg";
			if(sys::file_exists(module_path)) {
				std::string response = "{\nstatus: \"ok\",\nmodule: ";
				{
					std::string contents = sys::read_file(module_path);
					fprintf(stderr, "MANIFEST: %d\n", (int)doc.has_key("manifest"));
					if(doc.has_key("manifest")) {
						variant their_manifest = doc["manifest"];
						variant module = json::parse(contents);
						variant our_manifest = module["manifest"];

						std::vector<variant> deletions;
						for(auto p : their_manifest.as_map()) {
							if(!our_manifest.has_key(p.first)) {
								deletions.push_back(p.first);
							}
						}

						if(!deletions.empty()) {
							module.add_attr_mutation(variant("delete"), variant(&deletions));
						}

						std::vector<variant> matches;

						for(auto p : our_manifest.as_map()) {
							if(!their_manifest.has_key(p.first)) {
								fprintf(stderr, "their manifest does not have key: %s\n", p.first.write_json().c_str());
								continue;
							}

							if(p.second["md5"] != their_manifest[p.first]["md5"]) {
								fprintf(stderr, "their manifest mismatch key: %s\n", p.first.write_json().c_str());
								continue;
							}

							matches.push_back(p.first);
						}

						for(variant match : matches) {
							our_manifest.remove_attr_mutation(match);
						}

						contents = module.write_json();
					}

					response += contents;
				}

				response += "\n}";
				send_msg(socket, "text/json", response, "");

				variant summary = data_[module_id];
				if(summary.is_map()) {
					summary.add_attr_mutation(variant("num_downloads"), variant(summary["num_downloads"].as_int() + 1));
				}
				return;

			} else {
				response[variant("message")] = variant("No such module");
			}

		} else if(msg_type == "prepare_upload_module") {
			const std::string module_id = doc["module_id"].as_string();
			ASSERT_LOG(std::count_if(module_id.begin(), module_id.end(), isalnum) + std::count(module_id.begin(), module_id.end(), '_') == module_id.size(), "ILLEGAL MODULE ID");

			const std::string module_path = data_path_ + module_id + ".cfg";
			if(sys::file_exists(module_path)) {
				std::string contents = sys::read_file(module_path);
				variant module = json::parse(contents);
				variant manifest = module["manifest"];
				for(auto p : manifest.as_map()) {
					p.second.remove_attr_mutation(variant("data"));
				}

				response[variant("manifest")] = manifest;
			}

			module_lock_ids_[module_id] = next_lock_id_;
			response[variant("status")] = variant("ok");
			response[variant("lock_id")] = variant(next_lock_id_);

			++next_lock_id_;

		} else if(msg_type == "upload_module") {
			variant module_node = doc["module"];
			const std::string module_id = module_node["id"].as_string();
			ASSERT_LOG(std::count_if(module_id.begin(), module_id.end(), isalnum) + std::count(module_id.begin(), module_id.end(), '_') == module_id.size(), "ILLEGAL MODULE ID");

			variant lock_id = doc["lock_id"];
			ASSERT_LOG(lock_id == variant(module_lock_ids_[module_id]), "Invalid lock on module: " << lock_id.write_json() << " vs " << module_lock_ids_[module_id]);

			std::vector<variant> prev_versions;

			variant current_data = data_[variant(module_id)];
			if(current_data.is_null() == false) {
				const variant new_version = module_node[variant("version")];
				const variant old_version = current_data[variant("version")];
				ASSERT_LOG(new_version > old_version, "VERSION " << new_version.write_json() << " IS NOT NEWER THAN EXISTING VERSION " << old_version.write_json());
				prev_versions = current_data[variant("previous_versions")].as_list();
				current_data.remove_attr_mutation(variant("previous_versions"));
				prev_versions.push_back(current_data);
			}

			const std::string module_path = data_path_ + module_id + ".cfg";

			if(sys::file_exists(module_path)) {
				std::vector<variant> deletions;
				if(doc.has_key("delete")) {
					deletions = doc["delete"].as_list();
				}

				variant current_module = json::parse(sys::read_file(module_path));
				variant new_manifest = module_node["manifest"];
				variant old_manifest = current_module["manifest"];
				for(auto p : old_manifest.as_map()) {
					if(!new_manifest.has_key(p.first) && !std::count(deletions.begin(), deletions.end(), p.first)) {
						new_manifest.add_attr_mutation(p.first, p.second);
					}
				}
			}


			const std::string module_path_tmp = module_path + ".tmp";
			const std::string contents = module_node.write_json();
			
			sys::write_file(module_path_tmp, contents);
			const int rename_result = rename(module_path_tmp.c_str(), module_path.c_str());
			ASSERT_LOG(rename_result == 0, "FAILED TO RENAME FILE: " << errno);

			response[variant("status")] = variant("ok");

			{
				std::map<variant, variant> summary;
				summary[variant("previous_versions")] = variant(&prev_versions);
				summary[variant("version")] = module_node[variant("version")];
				summary[variant("name")] = module_node[variant("name")];
				summary[variant("description")] = module_node[variant("description")];
				summary[variant("author")] = module_node[variant("author")];
				summary[variant("dependencies")] = module_node[variant("dependencies")];
				summary[variant("num_downloads")] = variant(0);
				summary[variant("num_ratings")] = variant(0);
				summary[variant("sum_ratings")] = variant(0);

				std::vector<variant> reviews_list;
				summary[variant("reviews")] = variant(&reviews_list);

				if(module_node.has_key("icon")) {
					std::string icon_str = base64::b64decode(module_node["icon"].as_string());

					const std::string hash = md5::sum(icon_str);
					sys::write_file(data_path_ + "/.glob/" + hash, icon_str);

					summary[variant("icon")] = variant(hash);
				}

				data_.add_attr_mutation(variant(module_id), variant(&summary));
				write_data();
			}

		} else if(msg_type == "query_globs") {
			response[variant("status")] = variant("ok");
			foreach(const std::string& k, doc["keys"].as_list_string()) {
				const std::string data = sys::read_file(data_path_ + "/.glob/" + k);
				response[variant(k)] = variant(base64::b64encode(data));
			}
		} else if(msg_type == "rate") {
			const std::string module_id = doc["module_id"].as_string();
			variant summary = data_[module_id];
			ASSERT_LOG(summary.is_map(), "UNKNOWN MODULE ID: " << module_id);

			const int rating = doc["rating"].as_int();
			ASSERT_LOG(rating >= 1 && rating <= 5, "ILLEGAL RATING");
			summary.add_attr_mutation(variant("num_ratings"), variant(summary["num_ratings"].as_int() + 1));
			summary.add_attr_mutation(variant("sum_ratings"), variant(summary["sum_ratings"].as_int() + rating));

			if(doc["review"].is_null() == false) {
				std::vector<variant> v = summary["reviews"].as_list();
				v.push_back(doc);
				summary.add_attr_mutation(variant("reviews"), variant(&v));
			}

			response[variant("status")] = variant("ok");
		} else {
			ASSERT_LOG(false, "Unknown message type");
		}
	} catch(validation_failure_exception& e) {
		response[variant("status")] = variant("error");
		response[variant("message")] = variant(e.msg);
	}

	send_msg(socket, "text/json", variant(&response).write_json(), "");
}

void module_web_server::handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args)
{
	std::map<variant,variant> response;
	try {
		std::cerr << "URL: (" << url << ")\n";
		response[variant("status")] = variant("error");
		 if(url == "/get_summary") {
			response[variant("status")] = variant("ok");
			response[variant("summary")] = data_;
		} else {
			response[variant("message")] = variant("Unknown path");
		}
	} catch(validation_failure_exception& e) {
		response[variant("status")] = variant("error");
		response[variant("message")] = variant(e.msg);
	}

	send_msg(socket, "text/json", variant(&response).write_json(), "");
}

std::string module_web_server::data_file_path() const
{
	return data_path_ + "/module-data.json";
}

void module_web_server::write_data()
{
	const std::string tmp_path = data_file_path() + ".tmp";
	sys::write_file(tmp_path, data_.write_json());
	const int rename_result = rename(tmp_path.c_str(), data_file_path().c_str());
		ASSERT_LOG(rename_result == 0, "FAILED TO RENAME FILE: " << errno);
}

COMMAND_LINE_UTILITY(module_server)
{
	std::string path = ".";
	int port = 23456;

	std::deque<std::string> arguments(args.begin(), args.end());
	while(!arguments.empty()) {
		const std::string arg = arguments.front();
		arguments.pop_front();
		if(arg == "--path") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			path = arguments.front();
			arguments.pop_front();
		} else if(arg == "-p" || arg == "--port") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			port = atoi(arguments.front().c_str());
			arguments.pop_front();
		} else {
			ASSERT_LOG(false, "UNRECOGNIZED ARGUMENT: " << arg);
		}
	}

	const assert_recover_scope recovery;
	boost::asio::io_service io_service;
	module_web_server server(path, io_service, port);
	io_service.run();
}
