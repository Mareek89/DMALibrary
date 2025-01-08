#include <filesystem>
#include <fstream>
#include "consts.h"
#include "HidTable.h"
#include <WS2tcpip.h>

#include "kmbox.h"
#include "kmnetlogo.h"
#include "consts.h"

#pragma comment(lib, "Ws2_32.lib")

#define 	cmd_connect			0xaf3c2828
#define     cmd_mouse_move		0xaede7345
#define		cmd_mouse_left		0x9823AE8D
#define		cmd_mouse_middle	0x97a3AE8D
#define		cmd_mouse_right		0x238d8212
#define		cmd_mouse_wheel		0xffeead38
#define     cmd_mouse_automove	0xaede7346
#define     cmd_keyboard_all    0x123c2c2f
#define		cmd_reboot			0xaa8855aa
#define     cmd_bazerMove       0xa238455a
#define     cmd_monitor         0x27388020
#define     cmd_debug           0x27382021
#define     cmd_mask_mouse      0x23234343
#define     cmd_unmask_all      0x23344343
#define     cmd_setconfig       0x1d3d3323
#define     cmd_showpic         0x12334883

void kmbox::move_mouse(int x, int y) {
	if ((x == 0 && y == 0) || abs(x) > 100000 || abs(y) > 100000) return;

	if (inverted_mode) {
		switch (*inverted_mode) {
		case InvertedMode::NONE:
			break;
		case InvertedMode::INVERTED:
			x = -x;
			y = -y;
			break;
		case InvertedMode::INVERTED_X:
			x = -x;
			break;
		case InvertedMode::INVERTED_Y:
			y = -y;
			break;
		}
	}

	if (type == Type::NET) {
		kmnet_mouse.x = (short)x;
		kmnet_mouse.y = (short)y;

		kmbox::kmnet_send(cmd_mouse_move, (char*)&kmnet_mouse, sizeof(kmnet_mouse));

		kmnet_mouse.x = 0;
		kmnet_mouse.y = 0;
		return;
	}

	if (type == Type::MOUSEEVENT) {
		mouse_event(MOUSEEVENTF_MOVE, x, y, 0, 0);
		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.move(%d,%d)\r"), x, y);
	send_message(buffer);
}

void kmbox::side1(bool down) {
	if (type == Type::NET) {
		kmnet_mouse.button = (down ? (kmnet_mouse.button | 0x8) : (kmnet_mouse.button & (~0x8)));
		kmbox::kmnet_send(cmd_mouse_left, (char*)&kmnet_mouse, sizeof(kmnet_mouse));
		return;
	}

	if (type == Type::MOUSEEVENT) {
		mouse_event(down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP, 0, 0, XBUTTON1, 0);
		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.side1(%d)\r"), down);
	send_message(buffer);
}

void kmbox::side2(bool down) {
	if (type == Type::NET) {
		kmnet_mouse.button = (down ? (kmnet_mouse.button | 0x10) : (kmnet_mouse.button & (~0x10)));
		kmbox::kmnet_send(cmd_mouse_left, (char*)&kmnet_mouse, sizeof(kmnet_mouse));
		return;
	}

	if (type == Type::MOUSEEVENT) {
		mouse_event(down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP, 0, 0, XBUTTON2, 0);
		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.side2(%d)\r"), down);
	send_message(buffer);
}

void kmbox::left_click(bool down) {
	if (type == Type::NET) {
		kmnet_mouse.button = (down ? (kmnet_mouse.button | 0x01) : (kmnet_mouse.button & (~0x01)));
		kmbox::kmnet_send(cmd_mouse_left, (char*)&kmnet_mouse, sizeof(kmnet_mouse));
		return;
	}

	if (type == Type::MOUSEEVENT) {
		mouse_event(down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.left(%d)\r"), down);
	send_message(buffer);
}

void kmbox::right_click(bool down) {
	if (type == Type::NET) {
		kmnet_mouse.button = (down ? (kmnet_mouse.button | 0x02) : (kmnet_mouse.button & (~0x02)));
		kmbox::kmnet_send(cmd_mouse_right, (char*)&kmnet_mouse, sizeof(kmnet_mouse));
		return;
	}

	if (type == Type::MOUSEEVENT) {
		mouse_event(down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.right(%d)\r"), down);
	send_message(buffer);
}

void kmbox::middle_click(bool down) {
	if (type == Type::NET) {
		kmnet_mouse.button = (down ? (kmnet_mouse.button | 0x04) : (kmnet_mouse.button & (~0x04)));
		kmbox::kmnet_send(cmd_mouse_middle, (char*)&kmnet_mouse, sizeof(kmnet_mouse));
		return;
	}

	if (type == Type::MOUSEEVENT) {
		mouse_event(down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.middle(%d)\r"), down);
	send_message(buffer);
}


void kmbox::wheel(int count) {
	if (type == Type::NET) {
		kmnet_mouse.wheel = count;
		kmbox::kmnet_send(cmd_mouse_wheel, (char*)&kmnet_mouse, sizeof(kmnet_mouse));
		kmnet_mouse.wheel = 0;
		return;
	}

	if (type == Type::MOUSEEVENT) {
		mouse_event(MOUSEEVENTF_WHEEL, 0, 0, count, 0);
		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.wheel(%d)\r"), count);
	send_message(buffer);
}


void kmbox::key_click(int key, bool down) {
	// self defined keys
	switch (key) {
	case -3:
		left_click(down);
		return;
	case -2:
		right_click(down);
		return;
	case -1:
		middle_click(down);
		return;
	}

	if (type == Type::NET) {
		if (down) {
			kmnet_keydown(key);
		} else {
			kmnet_keyup(key);
		}

		return;
	}

	if (type == Type::MOUSEEVENT) {
		if (down) {
			keybd_event(key, 0, 0, 0);
		} else {
			keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
		}

		return;
	}

	char buffer[120]{};
	snprintf(buffer, 120, xorstr_("km.%s(%d)\r"), down ? xorstr_("down") : xorstr_("up"), key);
	send_message(buffer);
}

void kmbox::send_message(char* msg) {
	if (WriteFile(kmbox_handle, (void*)msg, (DWORD)strlen(msg), 0, NULL)) {
		return;
	}

	std::cout << xorstr_("[KMBox B+] Failed to send message\n");

	// restart connection
	kmbox::close();
	kmbox::open();
}

bool kmbox::is_open() {
	return kmbox_handle != 0;
}

void kmbox::close() {
	if (!kmbox_handle) return;

	if (type == Type::MOUSEEVENT) {
		type = Type::B_PRO;
		kmbox_handle = 0;
		return;
	}

	if (type == Type::NET) {
		closesocket(udpSocket);
		WSACleanup();

		type = Type::B_PRO;
		kmbox_handle = 0;
		return;
	}

	CloseHandle(kmbox_handle);
	kmbox_handle = 0;

	std::cout << xorstr_("[+] Closed kmbox device\n");
}

bool kmbox::open() {
	if (is_open()) return true;

	char port[120]{};
	strcat_s(port, xorstr_("\\\\.\\"));

	if (!scan_devices(xorstr_("USB-SERIAL CH340"), port) && !scan_devices(xorstr_("USB-SERIAL CP2102"), port)) {
		std::cout << xorstr_("[!] Failed to find KMBox B+.\n    - If you have a KMBox Net read ") << GUIDE_LINK << xorstr_("/kmboxnet\n    - If you have a KMBox B+ read ") << GUIDE_LINK << xorstr_("/kmboxb\n");
		return 0;
	}

	std::cout << xorstr_("[+] Found kmbox device: ") << port << xorstr_("\n");

	kmbox_handle = CreateFileA(
		port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (kmbox_handle == (HANDLE)-1) {
		std::cout << xorstr_("[!] Failed to open connection to KMBox\n");
		kmbox_handle = 0;
		return 0;
	}

	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(dcb);
	GetCommState(kmbox_handle, &dcb);
	
	dcb.BaudRate = CBR_115200;

	if (std::filesystem::exists(xorstr_("baudrate.txt"))) {
		// read baudrate and parse as number with error checking
		std::ifstream file(xorstr_("baudrate.txt"));
		if (file.is_open()) {
			std::string line;
			if (std::getline(file, line)) {
				try {
					dcb.BaudRate = std::stoi(line);
					std::cout << xorstr_("[+] Set baudrate to ") << dcb.BaudRate << xorstr_("\n");
				} catch (...) {
					std::cout << xorstr_("[!] Failed to parse baudrate\n");
				}
			}
		}
	}

	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;

	SetCommState(kmbox_handle, &dcb);

	COMMTIMEOUTS cto = { 0 };
	cto.ReadIntervalTimeout = 1;
	cto.ReadTotalTimeoutConstant = 0;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 0;
	cto.WriteTotalTimeoutMultiplier = 0;

	SetCommTimeouts(kmbox_handle, &cto);

	type = Type::B_PRO;
	return true;
}

// https://github.com/nbqofficial/cpp-arduino
bool kmbox::scan_devices(LPCSTR deviceName, LPSTR lpOut) {
	BOOL status = 0;
	char com[] = "COM";

	HDEVINFO deviceInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
	if (deviceInfo == INVALID_HANDLE_VALUE) { return false; }

	SP_DEVINFO_DATA dev_info_data;
	dev_info_data.cbSize = sizeof(dev_info_data);

	DWORD count = 0;

	std::cout << xorstr_("[+] Scanning for ") << deviceName << xorstr_("\n");
	while (SetupDiEnumDeviceInfo(deviceInfo, count++, &dev_info_data))
	{
		BYTE buffer[256];
		if (SetupDiGetDeviceRegistryProperty(deviceInfo, &dev_info_data, SPDRP_FRIENDLYNAME, NULL, buffer, sizeof(buffer), NULL))
		{
			LPCSTR lp_pos = strstr(reinterpret_cast<LPCSTR>(buffer), com);

			if (lp_pos && strstr(reinterpret_cast<LPCSTR>(buffer), deviceName))
			{
				DWORD i = static_cast<DWORD>(strlen(lpOut));
				DWORD len = i + static_cast<DWORD>(strlen(lp_pos));

				if (len < 120) {
					strncpy_s(lpOut + i, len - i, lp_pos, _TRUNCATE);
					lpOut[len - 1] = '\0';
					status = true;
					break;
				}
			}
		}
	}

	SetupDiDestroyDeviceInfoList(deviceInfo);
	return status;
}

void kmbox::kmnet_send(unsigned int message, const char* data, size_t data_size, unsigned int r) {
	CmdHead commandHeader = {};
	commandHeader.mac = kmnet_uuid;
	commandHeader.indexpts = txn_index++;
	commandHeader.cmd = message;
	commandHeader.rand = r;

	size_t size = sizeof(commandHeader) + data_size;
	char* buffer = new char[size];

	memcpy(buffer, (const char*)&commandHeader, sizeof(commandHeader));
	memcpy(buffer + sizeof(commandHeader), data, data_size);

	int sendresp = sendto(udpSocket, buffer, size, 0, reinterpret_cast<sockaddr*>(&kmbox_addr), sizeof(kmbox_addr));
	delete[] buffer;

	if (sendresp < 0) {
		std::cout << xorstr_("[!] Failed to send message to KMBox B+\n");
		close();
		return;
	}
}

void kmbox::kmnet_keyup(int vk_key) {
	int i;
	if (vk_key >= KEY_LEFTCONTROL && vk_key <= KEY_RIGHT_GUI) {
		switch (vk_key) {
		case KEY_LEFTCONTROL: kmnet_keyboard.ctrl &= ~BIT0; break;
		case KEY_LEFTSHIFT:   kmnet_keyboard.ctrl &= ~BIT1; break;
		case KEY_LEFTALT:     kmnet_keyboard.ctrl &= ~BIT2; break;
		case KEY_LEFT_GUI:    kmnet_keyboard.ctrl &= ~BIT3; break;
		case KEY_RIGHTCONTROL:kmnet_keyboard.ctrl &= ~BIT4; break;
		case KEY_RIGHTSHIFT:  kmnet_keyboard.ctrl &= ~BIT5; break;
		case KEY_RIGHTALT:    kmnet_keyboard.ctrl &= ~BIT6; break;
		case KEY_RIGHT_GUI:   kmnet_keyboard.ctrl &= ~BIT7; break;
		}
	} else {
		for (i = 0; i < 10; i++) {
			if (kmnet_keyboard.button[i] == vk_key) {
				memcpy(&kmnet_keyboard.button[i], &kmnet_keyboard.button[i + 1], 10 - i);
				kmnet_keyboard.button[9] = 0;
				goto KM_up_send;
			}
		}
	}
KM_up_send:
	kmbox::kmnet_send(cmd_keyboard_all, (char*)&kmnet_keyboard, sizeof(kmnet_keyboard));
}

void kmbox::kmnet_keydown(int vk_key) {
	int i;
	if (vk_key >= KEY_LEFTCONTROL && vk_key <= KEY_RIGHT_GUI)//¿ØÖÆ¼ü
	{
		switch (vk_key)
		{
		case KEY_LEFTCONTROL: kmnet_keyboard.ctrl |= BIT0; break;
		case KEY_LEFTSHIFT:   kmnet_keyboard.ctrl |= BIT1; break;
		case KEY_LEFTALT:     kmnet_keyboard.ctrl |= BIT2; break;
		case KEY_LEFT_GUI:    kmnet_keyboard.ctrl |= BIT3; break;
		case KEY_RIGHTCONTROL:kmnet_keyboard.ctrl |= BIT4; break;
		case KEY_RIGHTSHIFT:  kmnet_keyboard.ctrl |= BIT5; break;
		case KEY_RIGHTALT:    kmnet_keyboard.ctrl |= BIT6; break;
		case KEY_RIGHT_GUI:   kmnet_keyboard.ctrl |= BIT7; break;
		}
	} else {
		for (i = 0; i < 10; i++) {
			if (kmnet_keyboard.button[i] == vk_key) {
				goto KM_down_send;
			}
		}

		for (i = 0; i < 10; i++) {
			if (kmnet_keyboard.button[i] == 0) {
				kmnet_keyboard.button[i] = vk_key;
				goto KM_down_send;
			}
		}

		memcpy(&kmnet_keyboard.button[0], &kmnet_keyboard.button[1], 10);
		kmnet_keyboard.button[9] = vk_key;
	}
KM_down_send:
	kmbox::kmnet_send(cmd_keyboard_all, (char*)&kmnet_keyboard, sizeof(kmnet_keyboard));
}

bool kmbox::open_net(std::string& ip, int port, std::string& mac) {
	if (is_open()) {
		return false;
	}

	try {
		if (mac.length() != 8) {
			throw std::invalid_argument(xorstr_("MAC address must be 8 characters long."));
		}

		int result = 0;
		for (int i = 0; i < 4; i++) {
			result <<= 8;
			result |= std::stoi(mac.substr(i * 2, 2), 0, 16);
		}

		kmnet_uuid = result;
	} catch (...) {
		std::cout << xorstr_("[!] [KmNet] Invalid UUID/MAC Address\n");
		return false;
	}

	std::cout << xorstr_("[KMNet] Connecting...\n");

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << xorstr_("WSAStartup failed.\n");
		return false;
	}

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocket == INVALID_SOCKET) {
		std::cerr << xorstr_("Error creating socket: ") << WSAGetLastError() << xorstr_("\n");
		WSACleanup();
		return false;
	}

	kmbox_addr = {};
	kmbox_addr.sin_family = AF_INET;
	kmbox_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip.c_str(), &kmbox_addr.sin_addr);

	// Construct the command packet
	CmdHead commandHeader = {};
	commandHeader.mac = kmnet_uuid;
	commandHeader.indexpts = 0;
	commandHeader.rand = rand();
	commandHeader.cmd = cmd_connect;

	txn_index = 0;
	int res = sendto(udpSocket, reinterpret_cast<char*>(&commandHeader), sizeof(commandHeader), 0, reinterpret_cast<sockaddr*>(&kmbox_addr), sizeof(kmbox_addr));
	if (res <= 0) {
		closesocket(udpSocket);
		WSACleanup();

		std::cout << xorstr_("[KMNet] Failed to send init packet\n");
		return false;
	}

	std::cout << xorstr_("[KMNet] Waiting for response...\n");

	int timeout = 1000;
	setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	sockaddr_in from;
	int fromSize = sizeof(from);
	char responseBuffer[1024];
	int bytesRead = recvfrom(udpSocket, responseBuffer, sizeof(responseBuffer), 0, reinterpret_cast<sockaddr*>(&from), &fromSize);

	if (bytesRead < 0) {
		closesocket(udpSocket);
		WSACleanup();

		type = Type::B_PRO;
		kmbox::kmbox_handle = NULL;
		std::cout << xorstr_("[KMNet] Failed to connect\n");
		return false;
	}

	type = Type::NET;
	kmbox::kmbox_handle = (HANDLE)1; // handle is 1 to mark as connected

	std::cout << xorstr_("[KMNet] Successfully connected (") << bytesRead << xorstr_(")\n");

	kmnet_set_pic();
	return true;
}

void kmbox::kmnet_set_pic() {
	/*static int index = -1;

	index++;
	if (index >= 3) {
		index = 0;
	}

	const char* data = nullptr;
	switch (index) {
	case 0: data = (const char*)&kmnet_logo1; break;
	case 1: data = (const char*)&kmnet_logo2; break;
	case 2: data = (const char*)&kmnet_logo3; break;
	default:
		return;
	}*/

	const char* data = (const char*)&kmnet_logo1;

	for (int y = 00; y < 20; y++) {
		kmnet_send(cmd_showpic, &data[y * 1024], 1024, 80 + y * 4);

		// no fucking clue why this needs to be here, if its not it only renders half of the image
		if (y == 10) {
			Sleep(50);
		}
	}
}

void kmbox::open_mouse_event() {
	std::cout << xorstr_("[!!] Using MOUSE_EVENT commands instead of KMBox!");
	close();
	type = Type::MOUSEEVENT;
	kmbox_handle = (HANDLE)1;
}