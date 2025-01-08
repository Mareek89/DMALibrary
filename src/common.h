#pragma once

#include "kmbox.h"
#include "window.h"
#include "dma.h"
#include "socket.h"

#define SET_TITLE_C(TITLE) SetConsoleTitleA((xorstr_("Blurred.gg | ") + std::string(TITLE)).c_str())
#define SET_TITLE(TITLE) SET_TITLE_C(xorstr_(TITLE))

#ifdef _WINDLL
#define DLL true
#else
#define DLL false
#endif

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

#define CHECK_BIT(var,pos) ((var) & (1 << (pos)))
#define SET_BIT(var,pos,state) (state ? (var |= 1 << pos) : (var &= ~(1 << pos)))

namespace common {
	enum class CloseReason : uint8_t {
		UNKNOWN,
		USER_CLOSED,
		SESSION_TIMEOUT,
		PROTECTION_TIMEOUT,
		PROTECTION_FAILED
	};

	namespace state {
		inline CloseReason close_reason = CloseReason::UNKNOWN;
		inline bool running = true;
		inline int64_t expiry = 0;
		inline int64_t last_auth_keep_alive = 0;
		inline std::string config_section = xorstr_("");
		inline bool config_client = false;
		inline float debug_text_height = 0.f;
	}

	namespace settings {
		inline std::string game = xorstr_("fortnite");
		inline std::string version = xorstr_("1.00");
		inline void(*render_hook) = NULL;
		inline bool single_threaded_window = false;
		inline bool webmenu = true;
		inline bool* forced_vsync = nullptr;
	}

	inline uint64_t last_verified = 0;

	void pre_init();
	bool init();
	void post_init();
	void print_close_reason();

	const uint64_t now();

	void renderer();
	void debug_flag(std::string msg);

	uint64_t color_serializer(void* ptr);
	void color_processor(void* ptr, uint64_t col);

	void close_thread();

	void section(std::string section, bool client);
	void elm(std::string name, item_type type, void* ptr);
}

namespace cfg {
	namespace debug {
		inline bool hide_text = false;
		inline bool vsync = true;
		inline int monitor = -1;
		inline window::OverlayMode overlay = window::OverlayMode::DEFAULT;
		inline kmbox::InvertedMode inverted = kmbox::InvertedMode::NONE;
		inline bool kmbox_was_mouseevent = false;
	}

	namespace font {
		inline bool load_on_startup = false;
		inline std::string name = xorstr_("Arial");
		inline int size = 13;
	}

	namespace kmnet {
		inline std::string ip = xorstr_("192.168.2.188");
		inline std::string port = xorstr_("8000");
		inline std::string mac = xorstr_("00000000");
		inline bool start_on_launch = false;
	}

	namespace crosshair {
		inline bool enabled = false;
		inline float size = 10.f;
		inline float thickness = 2.f;
		inline ImU32 color = ImColor(255, 25, 25);
	}

	namespace colors {
		inline ImU32 text = ImColor(255, 255, 255);
		inline ImU32 background = ImColor(5, 5, 5);
		inline ImU32 accent = ImColor(194, 48, 48);
	}

}

