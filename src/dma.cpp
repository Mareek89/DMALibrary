#include "dma.h"
#include <fstream>
#include <algorithm>
#include <filesystem>

#include "xorstr.hpp"

#define DMA_SCATTER_FLAGS VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING_IO | VMMDLL_FLAG_NOCACHEPUT

int DMA::get_pid() {
    return pid;
}

bool DMA::init(bool printf) {
	std::vector<LPSTR> args;

    // if print.txt we want to print the output
    args.push_back(printf || std::filesystem::exists(xorstr_("print.txt")) ? (LPSTR)"-printf" : (LPSTR)"");
    args.push_back((LPSTR)"-disable-python");
    args.push_back((LPSTR)"-disable-symbolserver");
    args.push_back((LPSTR)"-device");
    args.push_back((LPSTR)"FPGA");

    LPSTR mmap = printf ? NULL : std::filesystem::exists(xorstr_("mmap.txt")) ? (LPSTR)"mmap.txt" : std::filesystem::exists(xorstr_("mmap.txt.txt")) ? (LPSTR)"mmap.txt.txt" : NULL;
    if (mmap != NULL) {
        std::cout << xorstr_("[!] Loading MMap\n");
        args.push_back((LPSTR)"");
        args.push_back((LPSTR)"-memmap");
        args.push_back(mmap);
    }

    if (manual_refresh) {
        args.push_back((LPSTR)"");
        args.push_back((LPSTR)"-norefresh");
    }

	handle = VMMDLL_Initialize(args.size(), args.data());

    if (!handle) {
        if (!printf) {
            return init(true);
        }

        std::cout << xorstr_("[!] Failed to initialize DMA\n\n");
        std::cout << xorstr_("[!!!] TO FIX ISSUE READ THIS GUIDE: ") << GUIDE_LINK << xorstr_("/troubleshooting\n");
        std::cout << xorstr_("[!!!] TO FIX ISSUE READ THIS GUIDE: ") << GUIDE_LINK << xorstr_("/troubleshooting\n");
        std::cout << xorstr_("[!!!] TO FIX ISSUE READ THIS GUIDE: ") << GUIDE_LINK << xorstr_("/troubleshooting\n\n");
        return false;
    }

    if (printf) {
        std::cout << xorstr_("[!!!] Continuing without your MMap. It is likely invalid.\n");
	}

    if (manual_refresh) {
        std::thread(&DMA::refresh_thread, this).detach();
    }

    return true;
}

int64_t DMA::get_now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void DMA::full_refresh() {
    if (manual_refresh) {
        VMMDLL_ConfigSet(handle, VMMDLL_OPT_REFRESH_ALL, 1);
    }
}

void DMA::proc_refresh() {
    if (manual_refresh) {
        VMMDLL_ConfigSet(handle, VMMDLL_OPT_REFRESH_FREQ_TLB, 1);
    }
}

void DMA::refresh_thread() {
    uint64_t last_slow = 0;
    uint64_t last_medium = 0;
    uint64_t last_fast = 0;

    while (handle != NULL) {
        if (wants_no_refresh) {
            break;
        }

        auto now = get_now();

        if (!last_fast) {
            last_slow = now;
			last_fast = now;
            continue;
		}

        if (wants_full_refresh) {
            if (now - last_slow > this->pid <= 0 ? int64_t(1000 * 5) : int64_t(1000 * 60 * 5)) {
                VMMDLL_ConfigSet(handle, VMMDLL_OPT_REFRESH_FREQ_SLOW, 1);
                last_slow = get_now();
            }
        } else {
            last_slow = 0;
        }

        if (wants_refresh) {
            if (now - last_medium > int64_t(1000 * 60)) {
				VMMDLL_ConfigSet(handle, VMMDLL_OPT_REFRESH_FREQ_MEDIUM, 1);
				last_medium = get_now();
			}
        } else {
			last_medium = 0;
		}

        if (now - last_fast > 5000) {
            VMMDLL_ConfigSet(handle, VMMDLL_OPT_REFRESH_FREQ_TLB_PARTIAL, 1);

            if (pid <= 0 || wants_full_refresh) {
                VMMDLL_ConfigSet(handle, VMMDLL_OPT_REFRESH_FREQ_MEM, 1);
            }

            last_fast = get_now();
        }

        Sleep(500);
    }
}

void DMA::set_refresh(bool do_refresh, bool force_no_refresh) {
    wants_refresh = do_refresh;
    wants_no_refresh = force_no_refresh;
}

void DMA::set_full_refresh(bool refresh) {
    wants_full_refresh = refresh;
}

void DMA::set_process(std::string new_proc_name) {

    process_name = new_proc_name;

    pid = -3;
    while (pid <= 0) {
        if (pid == -2 && process_name != "") {
            std::cout << "[!] Waiting for " << process_name << " to start\n";
        }

        Sleep(200);

        DWORD npid = 0;
        VMMDLL_PidGetFromName(handle, LPSTR(process_name.c_str()), &npid);
        if (!npid || npid < 0) {
            pid = pid == -3 ? -2 : -1;
            continue;
        }

        pid = npid;
    }
}

SectionInfo DMA::get_sections(uintptr_t base) {
    SectionInfo info = {};
    IMAGE_DOS_HEADER dosHeader = this->read<IMAGE_DOS_HEADER>(base);

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        return info;
    }

    uint64_t ntHeadersAddress = base + dosHeader.e_lfanew;
    IMAGE_NT_HEADERS ntHeaders = this->read<IMAGE_NT_HEADERS>(ntHeadersAddress);

    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
        return info;
    }

    uint64_t sectionHeadersAddress = ntHeadersAddress + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + ntHeaders.FileHeader.SizeOfOptionalHeader;

    for (int i = 0; i < ntHeaders.FileHeader.NumberOfSections; ++i) {
        IMAGE_SECTION_HEADER sectionHeader = this->read<IMAGE_SECTION_HEADER>(sectionHeadersAddress + i * sizeof(IMAGE_SECTION_HEADER));

        info.all.push_back({ base + sectionHeader.VirtualAddress, sectionHeader.Misc.VirtualSize });

        if (strncmp((char*)sectionHeader.Name, ".rdata", IMAGE_SIZEOF_SHORT_NAME) == 0) {
            info.rdata_base = base + sectionHeader.VirtualAddress;
            info.rdata_size = sectionHeader.Misc.VirtualSize;
            continue;
        }
        
        if (strncmp((char*)sectionHeader.Name, ".text", IMAGE_SIZEOF_SHORT_NAME) == 0) {
            info.text_base = base + sectionHeader.VirtualAddress;
            info.text_size = sectionHeader.Misc.VirtualSize;
            continue;
        }
    }

    return info;
}

void cbAddFile(HANDLE h, LPSTR uszName, ULONG64 cb, PVMMDLL_VFS_FILELIST_EXINFO pExInfo) {}

void DMA::read_cached(uintptr_t address, void* target, DWORD size) {
    VMMDLL_MemReadEx(handle, pid, address, reinterpret_cast<PBYTE>(target), size, nullptr, 0);
}

bool DMA::read_fixed(uintptr_t address, void* target, size_t size, int npid) {
    return VMMDLL_MemReadEx(handle, npid > 0 ? npid : pid, address, reinterpret_cast<PBYTE>(target), size, nullptr, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT);
}

void DMA::write_fixed(uintptr_t address, void* source, size_t size) {
    VMMDLL_MemWrite(handle, pid, address, reinterpret_cast<PBYTE>(source), size);
}

// scatter reads v2
ScatterHandle* DMA::new_scatter(int npid) {
    npid = npid <= 0 ? pid : npid;

    auto sHandle = VMMDLL_Scatter_Initialize(handle, npid, DMA_SCATTER_FLAGS);
    if (!sHandle) {
        return nullptr;
    }

    return new ScatterHandle(sHandle, this->handle, npid);
}

void DMA::close_scatter(ScatterHandle* handle) {
    VMMDLL_Scatter_CloseHandle(handle->get_handle());
    delete handle;
}

void ScatterHandle::clear() {
    VMMDLL_Scatter_Clear(handle, this->pid, DMA_SCATTER_FLAGS);
    this->list.clear();
    this->reads = 0;
    this->writes = 0;
}

bool ScatterHandle::execute() {
    if (this->reads == 0 && this->writes == 0) {
        return false;
    }

    const bool success = this->writes == 0 ? VMMDLL_Scatter_ExecuteRead(handle) : VMMDLL_Scatter_Execute(handle);
    if (!success) {
        this->clear();
		return false;
	}

    for (auto& read : this->list) {
        VMMDLL_Scatter_Read(this->handle, read.address, read.size, read.buffer, NULL);
	}

    this->clear();
	return true;
}

void ScatterHandle::read(uintptr_t address, void* target, DWORD size) {
    if (!VMMDLL_Scatter_Prepare(this->handle, address, size)) {
        return;
    }

    this->list.push_back({ address, size, (PBYTE)target });
    this->reads++;
}

void ScatterHandle::write(uintptr_t address, void* data, DWORD size) {
    if (VMMDLL_Scatter_PrepareWrite(this->handle, address, (PBYTE)data, size)) {
        this->writes++;
    }
}

// scatter reads
VMMDLL_SCATTER_HANDLE DMA::init_scatter(int npid) {
    return VMMDLL_Scatter_Initialize(handle, npid <= 0 ? pid : npid, DMA_SCATTER_FLAGS);
}

uintptr_t DMA::virt2phys(uintptr_t virtual_address, int npid) {
    if (virtual_address == 0) {
        return 0;
    }

    uintptr_t physical_address;
	bool success = VMMDLL_MemVirt2Phys(handle, npid <= 0 ? pid : npid, virtual_address, &physical_address);
    if (!success) {
        return 0;
	}

	return physical_address;
}

bool DMA::execute_scatter(VMMDLL_SCATTER_HANDLE sHandle) {
    if (!sHandle) {
        return false;
    }

    scatter_mutex.lock();
    bool exists = scatter_map.find(sHandle) != scatter_map.end();

    if (!exists) {
        scatter_mutex.unlock();
        VMMDLL_Scatter_CloseHandle(sHandle);
        return false;
    }

    auto list = scatter_map[sHandle];
    scatter_map.erase(sHandle);
    scatter_mutex.unlock();

    if (!list) {
        VMMDLL_Scatter_CloseHandle(sHandle);
        return false;
    }

    bool success = VMMDLL_Scatter_ExecuteRead(sHandle);
    if (!success) {
        VMMDLL_Scatter_CloseHandle(sHandle);
        return false;
    }

    size_t size = list->size();
    for (int i = 0; i < size; i++) {
        auto& read = list->at(i);

        // check if the buffer is valid write ptr
        if (!read.buffer || IsBadWritePtr(read.buffer, read.size)) {
			continue;
		}

        VMMDLL_Scatter_Read(sHandle, read.address, read.size, read.buffer, NULL);
	}

    VMMDLL_Scatter_CloseHandle(sHandle);
    delete list;

    return true;
}

void DMA::scatter_write(VMMDLL_SCATTER_HANDLE sHandle, uintptr_t address, void* data, DWORD size) {
    if (!sHandle) return;

	if (address < 0x1000) {
		return;
	}

	bool state = VMMDLL_Scatter_PrepareWrite(sHandle, address, (PBYTE)data, size);
	if (!state) {
		return;
	}
    
    // add a list if it doesnt exist to make sure it acknowledges the handle
	scatter_mutex.lock();
    if (scatter_map.find(sHandle) == scatter_map.end()) {
        scatter_map[sHandle] = new std::vector<ScatterRead>();
    }
	scatter_mutex.unlock();
}

void DMA::scatter_read(VMMDLL_SCATTER_HANDLE sHandle, uintptr_t address, void* target, DWORD size) {
    if (!sHandle) return;

    if (address < 0x1000) {
        return;
    }

    bool state = VMMDLL_Scatter_Prepare(sHandle, address, size);
    if (!state) {
        return;
    }

    scatter_mutex.lock();
    std::vector<ScatterRead>* list = nullptr;
    if (scatter_map.find(sHandle) != scatter_map.end()) {
        list = scatter_map[sHandle];
    } else {
        list = new std::vector<ScatterRead>();
        scatter_map[sHandle] = list;
    }

    list->push_back({ address, size, (PBYTE)target });
    scatter_mutex.unlock();
}

int DMA::scatter_size(VMMDLL_SCATTER_HANDLE sHandle) {
    scatter_mutex.lock();
    if (scatter_map.find(sHandle) != scatter_map.end()) {
        int size = scatter_map[sHandle]->size();
        scatter_mutex.unlock();
		return size;
    }
    scatter_mutex.unlock();

    return 0;
}

void DMA::close_scatter(VMMDLL_SCATTER_HANDLE sHandle) {
    scatter_mutex.lock();
    scatter_map.erase(sHandle);
    scatter_mutex.unlock();

    VMMDLL_Scatter_CloseHandle(sHandle);
}

// key handling

bool DMA::init_keystate(bool dont_print) {
    // force refresh of dtb/modules
    VMMDLL_ConfigSet(handle, VMMDLL_OPT_REFRESH_FREQ_TLB, 1);

    DWORD process_count = 0;
    PVMMDLL_PROCESS_INFORMATION list = NULL;
    if (VMMDLL_ProcessGetInformationAll(handle, &list, &process_count)) {
        bool seen = false;

        int force_pid = -1;
        if (std::filesystem::exists("keystate.txt")) {
            // read first line and parse it as int with error checking
            std::ifstream keystate_file("keystate.txt");
			std::string keystate_line;
			if (keystate_file.is_open()) {
				std::getline(keystate_file, keystate_line);
				keystate_file.close();

				try {
					force_pid = std::stoi(keystate_line);
				} catch (std::exception& e) {
					std::cout << "[!] Failed to parse keystate.txt: " << e.what() << "\n";
				}
			}

            if (force_pid <= 0 && force_pid != -1) {
				std::cout << "[!] Invalid PID in keystate.txt\n";
                force_pid = -1;
			}
        }

        for (int i = 0; i < process_count; i++) {
            PVMMDLL_PROCESS_INFORMATION entry = &list[i];

            if (force_pid > 0) {
                if (entry->dwPID != force_pid) {
                    continue;
                }
            } else {
                bool is_csrss = !strcmp(entry->szNameLong, "csrss.exe");
                if (!is_csrss && strcmp(entry->szNameLong, "winlogon.exe") && strcmp(entry->szNameLong, "explorer.exe") && strcmp(entry->szNameLong, "taskhostw.exe") && strcmp(entry->szNameLong, "smartscreen.exe") && strcmp(entry->szNameLong, "dwm.exe")) {
                    continue;
                }

                if (csrss_pid > 0) {
                    if (csrss_pid == entry->dwPID) {
                        continue;
                    }
                } else if (!seen && is_csrss) {
                    seen = true;
                    continue;
                }
            }

            bool win11 = false;

            ULONG64 addy = VMMDLL_ProcessGetProcAddressU(handle, entry->dwPID, (LPSTR)"win32kbase.sys", (LPSTR)"gafAsyncKeyState");
            if (!addy) {
                ULONG64 base = VMMDLL_ProcessGetModuleBaseU(handle, entry->dwPID, (LPSTR)"win32ksgd.sys");
                if (!base) {
                    continue;
                }

                uintptr_t session = read<uintptr_t>(read<uintptr_t>(read<uintptr_t>(base + 0x3110, entry->dwPID), entry->dwPID), entry->dwPID);
                if (!session) {
                    continue;
                }

                addy = session + 0x3690;
                win11 = true;
            }

            if (addy) {
                csrss_pid = entry->dwPID;
                gafAsyncKeyState = addy;

                if (DMA::update_keystate()) {
                    if (!dont_print) {
                        std::cout << "[!] Found " << entry->szNameLong << " (" << entry->dwPID << "): 0x" << std::hex << addy << std::dec << " (Windows " << (win11 ? "11" : "10") << ")\n";
                    }
                    VMMDLL_MemFree(list);
                    return true;
                }
            }
        }

        csrss_pid = NULL;
        gafAsyncKeyState = NULL;
        VMMDLL_MemFree(list);
    }

    return false;
}

bool DMA::update_keystate() {
    if (!csrss_pid || !gafAsyncKeyState) {
        return false;
    }

    auto prev_key_state_bitmap = key_state_bitmap;
    key_state_bitmap = read<std::array<std::uint8_t, 64>>(gafAsyncKeyState, csrss_pid);

    for (auto vk = 0u; vk < 256; ++vk) {
        if ((key_state_bitmap[(vk * 2 / 8)] & 1 << vk % 4 * 2) &&
            !(prev_key_state_bitmap[(vk * 2 / 8)] & 1 << vk % 4 * 2))
            key_state_recent_bitmap[vk / 8] |= 1 << vk % 8;
    }

    return true;
}

bool DMA::is_key_down(std::uint8_t const vk) {
    key_state_recent_bitmap[vk / 8] &= ~(1 << vk % 8);
    return key_state_bitmap[(vk * 2 / 8)] & 1 << vk % 4 * 2;
}

bool DMA::was_key_pressed(std::uint8_t const vk) {
    bool const result = key_state_recent_bitmap[vk / 8] & 1 << vk % 8;
    key_state_recent_bitmap[vk / 8] &= ~(1 << vk % 8);
    return result;
}

auto DMA::get_peb_address(const uint32_t process_id) const -> uint64_t
{
    VMMDLL_PROCESS_INFORMATION proc_info = { 0 };
    SIZE_T cb_proc_info = sizeof(VMMDLL_PROCESS_INFORMATION);

    proc_info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
    proc_info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
    proc_info.wSize = static_cast<WORD>(cb_proc_info);

    if (!VMMDLL_ProcessGetInformation(handle, process_id, &proc_info, &cb_proc_info))
    {
        std::cout << "Failed to get process information for PID: " << process_id << '\n';
        return 0;
    }

    const uint64_t peb_address = proc_info.win.vaPEB;

    return peb_address;
}

ModuleInfo DMA::get_module_base_address(std::string module_name_std, int npid, uintptr_t dtb_override) {
    if (npid <= 0) {
        npid = pid;
    }

    PVMMDLL_MAP_MODULEENTRY moduleEntry;
    LPSTR module_name = LPSTR(module_name_std.c_str());

    ModuleInfo info = {};

    bool result = VMMDLL_Map_GetModuleFromNameU(handle, npid, module_name, &moduleEntry, NULL);
    if (result) {
        info.base = moduleEntry->vaBase;
        return info;
    }

    return info;
}

void DMA::close() {
    VMMDLL_Close(handle);
    handle = NULL;
}

void DMA::override_pid(int target) {
    pid = target;
}