#pragma once

#include "consts.h"

#include <string>
#include <filesystem>
#include <iostream>
#include "xorstr.hpp"

class ByteBuffer {
private:
	std::vector<uint8_t> buffer;
	size_t read_pos = 0;
	uint8_t packet_id = 0;
public:
	// used for reading
	ByteBuffer(std::vector<uint8_t> bytes, bool is_server = true) {
		read_pos = 0;
		packet_id = read<uint8_t>();
	}

	// used for writing
	ByteBuffer(uint8_t packet_id) {
		buffer.push_back(packet_id);
		this->packet_id = packet_id;
	}

	ByteBuffer() {}

	~ByteBuffer() {
		buffer.clear();
	}

	template<typename T>
	void write(T value) {
		size_t size = sizeof(T);
		size_t currentPosition = buffer.size();
		buffer.resize(currentPosition + size);
		std::memcpy(&buffer[currentPosition], &value, size);
	}

	void write_raw(std::vector<uint8_t>* value) {
		buffer.insert(buffer.end(), value->begin(), value->end());
	}

	template<typename T>
	T read() {
		T value;
		size_t size = sizeof(T);
		if (read_pos + size <= buffer.size()) {
			std::memcpy(&value, &buffer[read_pos], size);
			read_pos += size;
		}
		return value;
	}

	void read_raw(std::vector<uint8_t>* value, size_t size) {
		value->insert(value->end(), buffer.begin() + read_pos, buffer.begin() + read_pos + size);
		read_pos += size;
	}

	uint8_t get_packet_id() {
		return packet_id;
	}

	void write_string(std::string value) {
		write<uint16_t>((uint16_t)value.length());

		std::vector<uint8_t> vec(value.begin(), value.end());
		buffer.insert(buffer.end(), vec.begin(), vec.end());
	}

	std::string read_string() {
		uint16_t length = read<uint16_t>();
		std::string value((char*)&buffer[read_pos], (int)length);
		read_pos += length;
		return value;
	}

public:
	std::vector<uint8_t>* get() {
		return &buffer;
	}

	size_t get_size() {
		return buffer.size();
	}
};

enum class item_type : uint8_t {
	INT,
	FLOAT,
	BOOLEAN,
	COLOR,
	UINT8,
	STRING,
	STRING_VECTOR,
	UINT8_VECTOR,
	DOUBLE,
	UINT64
};

struct config_item {
	std::string section;
	std::string name;
	item_type type;
	void* ptr;

	uint16_t item_id = 0;
};

// config manager
namespace cfgman {
	std::string get_app_folder();

	inline bool has_setup = false;

	inline std::string old_base = xorstr_("cfgs\\");
	inline std::string game = xorstr_("default");

	constexpr std::string client_id() {
		return game + xorstr_("_client");
	}

	struct config_mapping {
		std::string section;
		std::string old_name;
		std::string new_name;
	};

	inline std::vector<config_mapping> mappings = {}; // when we renamed config items we use a local mapping to make old configs work
	inline std::vector<config_item> elms = {}; // config item elements
	inline std::vector<config_item> client_elms = {}; // stored in a local file instead of on the server

	inline void(*on_config_update)() = nullptr;

	using ColorProcessor = void (*)(void*, uint64_t);
	inline ColorProcessor color_processor = nullptr;

	using ColorSerializer = uint64_t (*)(void*);
	inline ColorSerializer color_serializer = nullptr;

	void serialize_element(ByteBuffer* buf, config_item& item);
	ByteBuffer* serialize(std::vector<config_item>& list, bool write_version = true);
	void deserialize(ByteBuffer* buf, std::vector<config_item>& list);

	std::string get_file_path(std::string name);

	bool load_config_from_file(std::string id);

	bool save_config_to_file(std::string id);

	void save_config(std::string id);

	void load_config(std::string id);

	void delete_config(std::string id);

	void create_config(std::string name);

	// called after elms and client_elms have been filled and socket is connected
	void load();
}