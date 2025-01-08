#include <Windows.h>
#include "socket.h"
#include <fstream>
#include <vector>
#include <string>
#include <map>

/*
 config manager
*/

std::string cfgman::get_app_folder() {
	char szFileName[MAX_PATH];
	DWORD len = GetModuleFileName(NULL, szFileName, MAX_PATH);
	std::string result(szFileName, len);
	std::string::size_type pos = result.find_last_of("\\/");
	result.resize(pos + 1);
	return result;
}

std::string cfgman::get_file_path(std::string name) {
	// if ./cfgs doesnt exist, create it
	if (!std::filesystem::exists(old_base)) {
		std::filesystem::create_directory(old_base);
	}

	return std::string(old_base + name + ".wtf");
}

bool cfgman::load_config_from_file(std::string id) {
	auto path = get_file_path(id);
	if (!std::filesystem::exists(path)) {
		return false;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		std::cout << xorstr_("Failed to read config file\n");
		return false;
	}

	std::vector<uint8_t> vec((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	auto buf = new ByteBuffer();
	buf->write_raw(&vec);

	deserialize(buf, id == client_id() ? client_elms : elms);
	delete buf;

	std::cout << xorstr_("\r") << xorstr_("[+] Loaded config: ") << id << xorstr_("\n");
	return true;
}

bool cfgman::save_config_to_file(std::string id) {
	auto path = get_file_path(id);

	auto serialized = serialize(id == client_id() ? client_elms : elms, true);

	std::ofstream file(path, std::ios::binary);
	if (!file) {
		std::cout << xorstr_("[!] Failed to write config file\n");
		return false;
	}

	file.write((const char*)serialized->get()->data(), serialized->get_size());
	file.close();

	delete serialized;
	return true;
}

void cfgman::save_config(std::string id) {
	save_config_to_file(id);
}

void cfgman::load_config(std::string id) {
	load_config_from_file(id);
}

void cfgman::delete_config(std::string id) {
	auto path = get_file_path(id);
	if (std::filesystem::exists(path)) {
		std::filesystem::remove(path);
	}
}

void cfgman::create_config(std::string name) {
	save_config(name);
}

// called after elms and client_elms have been filled and socket is connected
void cfgman::load() {
	cfgman::has_setup = true;

	uint16_t index = 0;
	for (auto& elm : elms) {
		index++;
		elm.item_id = index;
	}

	for (auto& elm : client_elms) {
		index++;
		elm.item_id = index;
	}

	old_base = get_app_folder() + xorstr_("cfgs\\");

	// backwards compatibility for old client config paths
	std::string old_client_path = get_file_path(xorstr_("client"));
	if (std::filesystem::exists(old_client_path)) {
		std::string new_client_path = get_file_path(client_id());
		if (!std::filesystem::exists(new_client_path)) {
			std::filesystem::copy(old_client_path, new_client_path);
		}
	}

	load_config_from_file(client_id());

	if (cfgman::game != xorstr_("default")) {
		std::string old_path = get_file_path(xorstr_("default"));
		std::string new_path = get_file_path(cfgman::game);

		// dont delete old path, just in case they load xdefiant, and then go back to fortnite and their config is gone
		if (std::filesystem::exists(old_path) && !std::filesystem::exists(new_path)) {
			std::filesystem::copy(old_path, new_path);
		}
	}

	load_config_from_file(cfgman::game);
}

void cfgman::deserialize(ByteBuffer* buf, std::vector<config_item>& list) {
	uint8_t mode = buf->read<uint8_t>();
	if (mode > 0) {
		return;
	}

	auto sections = buf->read<uint16_t>();
	for (int i = 0; i < sections; i++) {
		auto section = buf->read_string();
		auto elms = buf->read<uint16_t>();

		for (int j = 0; j < elms; j++) {
			auto name = buf->read_string();
			auto type = buf->read<item_type>();

			bool found = false;
			for (auto& elm : list) {
				// dont check for mappings if it CANT possibly be mapped
				if (elm.section != section || elm.type != type) {
					continue;
				}

				if (elm.name != name) {
					bool is_mapped = false;

					for (auto& mapped : cfgman::mappings) {
						if (mapped.section == section && mapped.old_name == name && mapped.new_name == elm.name) {
							is_mapped = true;
							break;
						}
					}
					
					if (!is_mapped) {
						continue;
					}
				}

				found = true;
				switch (type) {
				case item_type::COLOR:
					if (color_processor) {
						color_processor(elm.ptr, buf->read<uint64_t>());
						break;
					}

					found = false;
					break;
				case item_type::BOOLEAN:
					*reinterpret_cast<bool*>(elm.ptr) = buf->read<bool>();
					break;
				case item_type::STRING:
					*reinterpret_cast<std::string*>(elm.ptr) = buf->read_string();
					break;
				case item_type::FLOAT:
					*reinterpret_cast<float*>(elm.ptr) = buf->read<float>();
					break;
				case item_type::INT:
					*reinterpret_cast<int*>(elm.ptr) = buf->read<int>();
					break;
				case item_type::UINT8:
					*reinterpret_cast<uint8_t*>(elm.ptr) = buf->read<uint8_t>();
					break;
				case item_type::DOUBLE:
						*reinterpret_cast<double*>(elm.ptr) = buf->read<double>();
						break;
				case item_type::STRING_VECTOR:
				{
					auto size = buf->read<uint16_t>();
					auto vec = std::vector<std::string>();
					for (int i = 0; i < size; i++) {
						vec.push_back(buf->read_string());
					}

					*reinterpret_cast<std::vector<std::string>*>(elm.ptr) = vec;
				}
				break;
				case item_type::UINT8_VECTOR:
				{
					auto size = buf->read<uint16_t>();
					auto vec = std::vector<uint8_t>();
					for (int i = 0; i < size; i++) {
						vec.push_back(buf->read<uint8_t>());
					}

					*reinterpret_cast<std::vector<uint8_t>*>(elm.ptr) = vec;
				}
				break;
				}
			
				break;
			}

			if (!found) {
				// read the byte counts for the type if the element has been removed
				switch (type) {
				case item_type::COLOR:
					buf->read<uint64_t>();
					break;
				case item_type::BOOLEAN:
					buf->read<bool>();
					break;
				case item_type::STRING:
					buf->read_string();
					break;
				case item_type::FLOAT:
					buf->read<float>();
					break;
				case item_type::INT:
					buf->read<int>();
					break;
				case item_type::UINT8:
					buf->read<uint8_t>();
					break;
				case item_type::DOUBLE:
					buf->read<double>();
					break;
				case item_type::STRING_VECTOR:
				{
					auto size = buf->read<uint16_t>();
					for (int i = 0; i < size; i++) {
						buf->read_string();
					}
				}
				break;
				case item_type::UINT8_VECTOR:
				{
					auto size = buf->read<uint16_t>();
					for (int i = 0; i < size; i++) {
						buf->read<uint8_t>();
					}
				}
				break;
				}
			}
		}
	}

	if (on_config_update) {
		on_config_update();
	}
}

void cfgman::serialize_element(ByteBuffer* buf, config_item& elm) {
	buf->write<item_type>(elm.type);

	switch (elm.type) {
	case item_type::COLOR:
		buf->write<uint64_t>(color_serializer(elm.ptr));
		break;
	case item_type::BOOLEAN:
		buf->write<bool>(*reinterpret_cast<bool*>(elm.ptr));
		break;
	case item_type::STRING:
		buf->write_string(*reinterpret_cast<std::string*>(elm.ptr));
		break;
	case item_type::FLOAT:
		buf->write<float>(*reinterpret_cast<float*>(elm.ptr));
		break;
	case item_type::INT:
		buf->write<int>(*reinterpret_cast<int*>(elm.ptr));
		break;
	case item_type::UINT8:
		buf->write<uint8_t>(*reinterpret_cast<uint8_t*>(elm.ptr));
		break;
	case item_type::DOUBLE:
		buf->write<double>(*reinterpret_cast<double*>(elm.ptr));
		break;
	case item_type::STRING_VECTOR:
	{
		auto vec = reinterpret_cast<std::vector<std::string>*>(elm.ptr);
		buf->write<uint16_t>(uint16_t(vec->size()));
		for (auto& str : *vec) {
			buf->write_string(str);
		}
	}
	break;
	case item_type::UINT8_VECTOR:
	{
		auto vec = *reinterpret_cast<std::vector<uint8_t>*>(elm.ptr);
		buf->write<uint16_t>(uint16_t(vec.size()));
		for (auto& str : vec) {
			buf->write<uint8_t>(str);
		}
	}
	break;
	}
}

ByteBuffer* cfgman::serialize(std::vector<config_item>& list, bool write_version) {
	// sort list into sections
	std::map<std::string, std::vector<config_item>> sections = {};
	for (auto& elm : list) {
		if (sections.find(elm.section) == sections.end()) {
			sections[elm.section] = {};
		}

		sections[elm.section].push_back(elm);
	}

	auto buf = new ByteBuffer();
	// write non compressed
	if (write_version) {
		buf->write<uint8_t>(0);
	}

	buf->write<uint16_t>(uint16_t(sections.size()));
	for (auto& [section, elms] : sections) {
		buf->write_string(section);
		buf->write<uint16_t>(uint16_t(elms.size()));
		for (auto& elm : elms) {
			buf->write_string(elm.name);
			serialize_element(buf, elm);
		}
	}

	return buf;
}