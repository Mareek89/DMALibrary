#pragma once

#include <iostream>
#include <array>
#include <windows.h>
#include "leechcore.h"
#include "consts.h"
#include "vmmdll.h"
#include <vector>
#include <mutex>
#include <map>
#include <string>
#pragma comment(lib, "leechcore")
#pragma comment(lib, "vmm")

typedef struct ScatterRead {
	uintptr_t address;
	size_t size;
	PBYTE buffer;
} ScatterRead;

class ScatterHandle {
private:

	VMMDLL_SCATTER_HANDLE handle;
	VMM_HANDLE vmm_handle;
	int pid = 0;

	std::vector<ScatterRead> list;
	int reads = 0;
	int writes = 0;

public:
	std::string name = "None";

	ScatterHandle(VMMDLL_SCATTER_HANDLE handle, VMM_HANDLE vmm_handle, int pid) : handle(handle), vmm_handle(vmm_handle), pid(pid) {}


	VMMDLL_SCATTER_HANDLE get_handle() {
		return handle;
	}

	VMM_HANDLE get_vmm_handle() {
		return vmm_handle;
	}

	int get_reads() {
		return reads;
	}

	int get_writes() {
		return writes;
	}

	bool execute();
	void read(uintptr_t address, void* target, DWORD size);
	void write(uintptr_t address, void* data, DWORD size);
	void clear();

	template<typename T>
	void read(uintptr_t address, T* target) {
		read(address, target, sizeof(T));
	}
	
	template<typename T>
	void write(uintptr_t address, T data) {
		write(address, &data, sizeof(T));
	}
};

struct ModuleInfo {
	uint32_t index;
	uint32_t process_id;
	uint64_t dtb;
	uint64_t kernelAddr;
	uint64_t base;
	std::string name;
};

struct SectionInfo {
	uintptr_t text_base;
	DWORD text_size;

	uintptr_t rdata_base;
	DWORD rdata_size;

	std::vector<std::pair<uintptr_t, DWORD>> all;
};

struct ScanResult {
	uintptr_t address;
	size_t index;
	char* buffer;
	size_t buffer_size;
};

class DMA {
private:
	int pid = NULL;
	std::string process_name;
	VMM_HANDLE handle = NULL;
	
	// scatter api
	std::mutex scatter_mutex;

	// map of scatter handle -> address & size list
	std::map<VMMDLL_SCATTER_HANDLE, std::vector<ScatterRead>*> scatter_map;

	// key handling
	int csrss_pid = NULL;
	uintptr_t gafAsyncKeyState = NULL;
	std::array<std::uint8_t, 64> key_state_bitmap = {};
	std::array<std::uint8_t, 32> key_state_recent_bitmap = {};

	bool manual_refresh = false;
	bool wants_refresh = true;
	bool wants_full_refresh = true;
	bool wants_no_refresh = false;

	void refresh_thread();
public:
	DMA() {}

	~DMA() {
		if (handle) {
			VMMDLL_Close(handle);
		}

		for (auto& [handle, list] : scatter_map) {
			delete list;
		}

		scatter_map.clear();
	}

	void set_manual_refresh(bool manual) {
		if (handle == NULL) {
			manual_refresh = manual;
		}
	}

	bool do_manual_refresh() const {
		return manual_refresh;
	}
	
	/*
		target process handling
	*/

	bool init(bool printf = false);
	void set_process(std::string process_name);
	void set_refresh(bool do_refresh, bool force_no_refresh);
	void set_full_refresh(bool do_refresh);
	int64_t get_now();
	void full_refresh();
	void proc_refresh();
	void close();
	int get_pid();
	void override_pid(int pid);
	ModuleInfo get_module_base_address(std::string module_name, int npid = -1, uintptr_t dtb_override = 0);
	uintptr_t virt2phys(uintptr_t address, int npid = -1);
	SectionInfo get_sections(uintptr_t base);

	/*
		scatter reads v2
	*/

	ScatterHandle* new_scatter(int pid = -1);
	void close_scatter(ScatterHandle* sHandle);

	/*
		scatter reads v1
	*/

	VMMDLL_SCATTER_HANDLE init_scatter(int pid = -1);
	void close_scatter(VMMDLL_SCATTER_HANDLE sHandle);
	bool execute_scatter(VMMDLL_SCATTER_HANDLE sHandle);
	int scatter_size(VMMDLL_SCATTER_HANDLE sHandle);
	void scatter_read(VMMDLL_SCATTER_HANDLE sHandle, uintptr_t address, void* target, DWORD size);
	void scatter_write(VMMDLL_SCATTER_HANDLE sHandle, uintptr_t address, void* data, DWORD size);

	template<typename T>
	void scatter_read(VMMDLL_SCATTER_HANDLE sHandle, uintptr_t address, T* target) {
		scatter_read(sHandle, address, target, sizeof(T));
	}

	template<typename T>
	void scatter_write(VMMDLL_SCATTER_HANDLE sHandle, uintptr_t address, T data) {
		scatter_write(sHandle, address, &data, sizeof(T));
	}

	/*
		single reads & writes
	*/
	template<typename T>
	void write(uintptr_t address, T value) {
		VMMDLL_MemWrite(handle, pid, address, reinterpret_cast<PBYTE>(&value), sizeof(T));
	}

	template<typename T>
	T read(uintptr_t address, int npid = -1) {
		T value;
		VMMDLL_MemReadEx(handle, npid > 0 ? npid : pid, address, reinterpret_cast<PBYTE>(&value), sizeof(T), nullptr, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT);
		return value;
	}

	template<typename T>
	T read_cached(uintptr_t address) {
		T value;
		VMMDLL_MemReadEx(handle, pid, address, reinterpret_cast<PBYTE>(&value), sizeof(T), nullptr, 0);
		return value;
	}

	void read_cached(uintptr_t address, void* target, DWORD size);

	bool read_fixed(uintptr_t address, void* target, size_t size, int npid = -1);
	void write_fixed(uintptr_t address, void* data, size_t size);

	/*
		external key handling
	*/
	bool init_keystate(bool dont_print = false);
	bool update_keystate();
	bool is_key_down(std::uint8_t const vk);
	bool was_key_pressed(std::uint8_t const vk);
	[[nodiscard]] auto get_peb_address(uint32_t process_id) const -> uint64_t;

	// retrieving handle
	VMM_HANDLE get_handle() {
		return handle;
	}
};

constexpr std::array<std::string_view, 256> g_key_codes {
	"Unknown",
	"Left Mouse",
	"Right Mouse",
	"Control-break",
	"Middle Mouse",
	"Mouse 4",
	"Mouse 5",
	"Undefined",
	"Backspace",
	"TAB",
	"Reserved",
	"Reserved",
	"Clear",
	"Enter",
	"Reserved",
	"Reserved",
	"Shift",
	"Control",
	"Alt",
	"Pause",
	"Caps Lock",
	"IME",
	"IME",
	"IME",
	"IME",
	"IME",
	"IME",
	"ESC",
	"IME",
	"IME",
	"IME",
	"IME",
	"Space",
	"Page Up",
	"Page Down",
	"End",
	"Home",
	"Left Arrow",
	"Up Arrow",
	"Right Arrow",
	"Down Arrow",
	"Select",
	"Print",
	"Execute",
	"Print Screen",
	"Insert",
	"Delete",
	"Help",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"Left Window",
	"Right Window",
	"Applications",
	"Reserved",
	"Sleep",
	"Numpad 0",
	"Numpad 1",
	"Numpad 2",
	"Numpad 3",
	"Numpad 4",
	"Numpad 5",
	"Numpad 6",
	"Numpad 7",
	"Numpad 8",
	"Numpad 9",
	"Numpad *",
	"Numpad +",
	"Numpad Separator",
	"Numpad -",
	"Numpad .",
	"Numpad /",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"F13",
	"F14",
	"F15",
	"F16",
	"F17",
	"F18",
	"F19",
	"F20",
	"F21",
	"F22",
	"F23",
	"F24",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Numlock",
	"Scroll",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"Left Shift",
	"Right Shift",
	"Left Control",
	"Right Control",
	"Left Menu",
	"Right Menu",
	"Browser back",
	"Browser forward",
	"Browser refresh",
	"Browser stop",
	"Browser search",
	"Browser favorites",
	"Browser home",
	"Volume mute",
	"Volume down",
	"Volume up",
	"Next track",
	"Previous track",
	"Stop track",
	"Play/Pause track",
	"Start mail",
	"Select media",
	"Start app1",
	"Start app2",
	"Reserved",
	"Reserved",
	";:",
	"+",
	",",
	"-",
	".",
	"/?",
	"`~",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Unassigned",
	"Unassigned",
	"Unassigned",
	"[{",
	"\\|",
	"]}",
	"\'\"",
	"OEM",
	"Reserved",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"IME",
	"OEM",
	"VK_PACKET",
	"Unassigned",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"OEM",
	"Attn",
	"CrSel",
	"ExSel",
	"EOF",
	"Play",
	"Zoom",
	"Noname",
	"PA1",
	"Clear",
	"None"
};

constexpr const std::string get_key_name(const std::size_t key_code) {
	std::string_view view = g_key_codes.at(key_code < g_key_codes.size() ? key_code : 0u);
	return std::string(view.data(), view.size());
}