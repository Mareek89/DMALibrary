#include "common.h"

void common::pre_init() {
	srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());

	SetConsoleOutputCP(CP_UTF8);
	SET_TITLE("Stage #1");

	std::thread(close_thread).detach();
}

bool common::init() {
	SET_TITLE("Stage #6");

	cfgman::color_serializer = color_serializer;
	cfgman::color_processor = color_processor;

	SET_TITLE("Stage #7");

	section(xorstr_("cmn"), true);
	elm(xorstr_("v"), item_type::BOOLEAN, &cfg::debug::vsync);
	elm(xorstr_("m"), item_type::INT, &cfg::debug::monitor);
	elm(xorstr_("o"), item_type::UINT8, &cfg::debug::overlay);
	elm(xorstr_("h"), item_type::BOOLEAN, &cfg::debug::hide_text);
	elm(xorstr_("i"), item_type::UINT8, &cfg::debug::inverted);
	elm(xorstr_("e"), item_type::BOOLEAN, &cfg::debug::kmbox_was_mouseevent);
	elm(xorstr_("f"), item_type::INT, &window::fps_limit);

	elm(xorstr_("kl"), item_type::BOOLEAN, &cfg::kmnet::start_on_launch);
	elm(xorstr_("ki"), item_type::STRING, &cfg::kmnet::ip);
	elm(xorstr_("kp"), item_type::STRING, &cfg::kmnet::port);
	elm(xorstr_("km"), item_type::STRING, &cfg::kmnet::mac);

	elm(xorstr_("fs"), item_type::BOOLEAN, &cfg::font::load_on_startup);
	elm(xorstr_("fn"), item_type::STRING, &cfg::font::name);
	elm(xorstr_("fz"), item_type::INT, &cfg::font::size);

	elm(xorstr_("ce"), item_type::BOOLEAN, &cfg::crosshair::enabled);
	elm(xorstr_("cs"), item_type::FLOAT, &cfg::crosshair::size);
	elm(xorstr_("ct"), item_type::FLOAT, &cfg::crosshair::thickness);
	elm(xorstr_("cc"), item_type::COLOR, &cfg::crosshair::color);

	elm(xorstr_("ot"), item_type::COLOR, &cfg::colors::text);
	elm(xorstr_("oa"), item_type::COLOR, &cfg::colors::accent);
	elm(xorstr_("ob"), item_type::COLOR, &cfg::colors::background);

	SET_TITLE("Stage #8");
	cfgman::game = settings::game;
	cfgman::load();

	SET_TITLE("Stage #9");

	if (cfg::kmnet::start_on_launch) {
		int port = -1;
		try {
			port = std::stoi(cfg::kmnet::port);
		} catch (const std::invalid_argument& e) {
			std::cerr << xorstr_("[!] [KmNet Port] Invalid argument: ") << e.what() << "\n";
		} catch (const std::out_of_range& e) {
			std::cerr << xorstr_("[!] [KmNet Port] Out of range: ") << e.what() << "\n";
		}

		kmbox::open_net(cfg::kmnet::ip, port, cfg::kmnet::mac);
	}

	if (!kmbox::is_open() && kmbox::open()) {
		cfg::kmnet::start_on_launch = false;
	}

	kmbox::inverted_mode = &cfg::debug::inverted;

	if (!kmbox::is_open() && cfg::debug::kmbox_was_mouseevent) {
		kmbox::open_mouse_event();
	} else {
		cfg::debug::kmbox_was_mouseevent = false;
	}

	return true;
}

void common::post_init() {
	window::render_hook = renderer;

	window::target_monitor = cfg::debug::monitor;
	window::vsync = settings::forced_vsync ? *settings::forced_vsync : cfg::debug::vsync;
	window::overlay = cfg::debug::overlay;

	if (cfg::font::load_on_startup) {
		window::load_font(cfg::font::name, cfg::font::size, true, false);
	}

	if (!settings::single_threaded_window) {
		std::thread(window::init, &state::running).detach();

		while (!window::is_setup && state::running) {
			Sleep(5);
		}
	} else {
		window::init(&state::running);
	}
}

void common::renderer() {
	state::debug_text_height = 5.f;

	if (settings::render_hook) {
		((void(*)(void))settings::render_hook)();
	}

	if (cfg::crosshair::enabled) {
		const float size = cfg::crosshair::size / 2.f;

		auto draw = ImGui::GetBackgroundDrawList();
		draw->AddLine({ (float)window::wx / 2 - size, (float)window::wy / 2 }, { (float)window::wx / 2 + size, (float)window::wy / 2 }, cfg::crosshair::color, cfg::crosshair::thickness);
		draw->AddLine({ (float)window::wx / 2, (float)window::wy / 2 - size }, { (float)window::wx / 2, (float)window::wy / 2 + size }, cfg::crosshair::color, cfg::crosshair::thickness);
	}
}

void common::debug_flag(std::string msg) {
	const char* text = msg.c_str();
	window::outlined_text({ 5, state::debug_text_height }, cfg::colors::text, text);
	state::debug_text_height += ImGui::CalcTextSize(text).y + 1;
}

uint64_t common::color_serializer(void* ptr) {
	return (uint64_t)*reinterpret_cast<ImU32*>(ptr);
}

void common::color_processor(void* ptr, uint64_t col) {
	*reinterpret_cast<ImU32*>(ptr) = (ImU32)col;
}

void common::close_thread() {
	while (!GetAsyncKeyState(VK_END)) {
		Sleep(100);
	}

	state::close_reason = CloseReason::USER_CLOSED;
	state::running = false;
}

void common::section(std::string section, bool client) {
	state::config_section = section;
	state::config_client = client;
}

void common::elm(std::string name, item_type type, void* ptr) {
	if (state::config_client) {
		cfgman::client_elms.push_back({ state::config_section, name, type, ptr });
	} else {
		cfgman::elms.push_back({ state::config_section, name, type, ptr });
	}
}

const uint64_t common::now() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void common::print_close_reason() {
	switch (state::close_reason) {
	case CloseReason::SESSION_TIMEOUT:
		std::cout << xorstr_("[-] Session timed out!\n");
		break;
	case CloseReason::USER_CLOSED:
		std::cout << xorstr_("[-] User exited!\n");
		break;
	case CloseReason::PROTECTION_FAILED:
		std::cout << xorstr_("[-] Unknown Error Occoured #1\n");
		break;
	case CloseReason::PROTECTION_TIMEOUT:
		std::cout << xorstr_("[+] Unknown Error Occoured #2\n");
		break;
	default:
		std::cout << xorstr_("[+] Unknown Close Reason\n");
		break;
	}
}