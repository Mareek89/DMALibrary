#pragma once

#include <Windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include "xorstr.hpp"

#pragma comment (lib, "SetupAPI.lib")

// https://docs.google.com/document/d/1aIHqH2n4UN7tAYOu3-Mhr3qvvhSWeA-SrraSkSx-hQI/edit

static GUID GUID_DEVCLASS_PORTS = { 0x4d36e978, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };

struct CmdHead
{
    unsigned int  mac;
    unsigned int  rand;
    unsigned int  indexpts;
    unsigned int  cmd;
};

struct SoftMouse
{
    int button;
    int x;
    int y;
    int wheel;
    int point[10];
};

struct SoftKeyboard
{
    char ctrl;
    char resvel;
    char button[10];
};

namespace kmbox {
	static HANDLE kmbox_handle = 0;

    enum class InvertedMode : uint8_t {
        NONE,
        INVERTED,
        INVERTED_X,
        INVERTED_Y,
    };

    enum class Type : uint8_t {
        B_PRO,
        NET,
        MOUSEEVENT,
    };

    inline Type type = Type::B_PRO;
    inline InvertedMode* inverted_mode = nullptr;

    // kmnet stuff
    static int kmnet_uuid = 0;
    static int txn_index = 0;
    static sockaddr_in kmbox_addr;
    static SOCKET udpSocket;
    static SoftKeyboard kmnet_keyboard = {};
    static SoftMouse kmnet_mouse = {};

    static const std::map<std::string, int> key_map = {
        // self defined
        {xorstr_("Left Click"), -3},
        {xorstr_("Right Click"), -2},
        {xorstr_("Middle Click"), -1},

        // kmbox keys
        {xorstr_("Space"), 44}, {xorstr_("A"), 4}, {xorstr_("B"), 5}, {xorstr_("D"), 7}, {xorstr_("2"), 31}, {xorstr_("4"), 33}, {xorstr_("6"), 35}, {xorstr_("8"), 37}, {xorstr_("0"), 39},
        {xorstr_("C"), 6}, {xorstr_("E"), 8}, {xorstr_("F"), 9}, {xorstr_("G"), 10},{xorstr_("H"), 11}, {xorstr_("I"), 12},{xorstr_("J"), 13}, {xorstr_("K"), 14}, {xorstr_("L"), 15}, {xorstr_("M"), 16},
        {xorstr_("N"), 17}, {xorstr_("O"), 18}, {xorstr_("P"), 19}, {xorstr_("Q"), 20}, {xorstr_("R"), 21},
        {xorstr_("T"), 23}, {xorstr_("V"), 25}, {xorstr_("W"), 26}, {xorstr_("X"), 27}, {xorstr_("Y"), 28}, {xorstr_("Tab"), 43}, {xorstr_("Enter"), 40}, {xorstr_("Esc"), 41},
        {xorstr_("F1"), 58}, {xorstr_("F2"), 59}, {xorstr_("F3"), 60}, {xorstr_("F4"), 61}, {xorstr_("F5"), 62}, {xorstr_("F6"), 63}, {xorstr_("F7"), 64},
        {xorstr_("F8"), 65}, {xorstr_("F9"), 66}, {xorstr_("F10"), 67}, {xorstr_("F11"), 68}, {xorstr_("F12"), 69}, {xorstr_("Up"), 82}, {xorstr_("Down"), 81},
        {xorstr_("Left"), 80}, {xorstr_("Right"), 79}, {xorstr_("-"), 45}, {xorstr_("="), 46}, {xorstr_("Insert"), 73}, {xorstr_("Home"), 74}, {xorstr_("["), 47},
        {xorstr_("]"), 48}, {xorstr_("\\"), 49}, {xorstr_("End"), 77}, {xorstr_("Caps"), 57}, {xorstr_(";"), 51}, {xorstr_("'"), 52}, {xorstr_("),"), 54}, {xorstr_("."), 55},
        {xorstr_("/"), 56}, {xorstr_("Print"), 70}, {xorstr_("Scroll"), 71}, {xorstr_("Pause"), 72}, {xorstr_("PgUp"), 75}, {xorstr_("F13"), 104}, {xorstr_("F14"), 105},
        {xorstr_("F15"), 106}, {xorstr_("F16"), 107}, {xorstr_("F17"), 108}, {xorstr_("F18"), 109}, {xorstr_("F19"), 110}, {xorstr_("F20"), 111},
        {xorstr_("PgDown"), 78}, {xorstr_("Aplcat"), 101}, {xorstr_("Ctr-L"), 224}, {xorstr_("Shift-L"), 225}, {xorstr_("Alt-L"), 226},
        {xorstr_("Gui-L"), 227}, {xorstr_("Ctr-R"), 228}, {xorstr_("Shift-R"), 229}, {xorstr_("Alt-R"), 230}, {xorstr_("Gui-R"), 231}, {xorstr_("Help"), 254}
    };
    static const std::vector<std::pair<std::string, int>> key_vector(key_map.begin(), key_map.end());

	void move_mouse(int x, int y);
	void left_click(bool down);
	void right_click(bool down);
    void middle_click(bool down);
    void side1(bool down);
    void side2(bool down);
    void wheel(int count);
	void key_click(int key, bool down);
    void send_message(char* msg);

	void close();
	bool open();
    bool open_net(std::string& ip, int port, std::string& mac);
    void open_mouse_event();
	bool is_open();
	bool scan_devices(LPCSTR deviceName, LPSTR lpOut);

    // only used internally
    void kmnet_keydown(int key);
    void kmnet_keyup(int key);
    void kmnet_send(unsigned int message, const char* data, size_t size, unsigned int r = rand());
    void kmnet_set_pic();
}