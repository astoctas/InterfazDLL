// dllmain.cpp : Define el punto de entrada de la aplicación DLL.
#include "stdafx.h"
#include "winuser.h"
#include "I2Cplugin.h"
#include <thread>
#include <string>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <iostream>

using namespace std;

#define DllExport __declspec( dllexport )
#define FIRMATA_PIN_MODE_SERVO 0x04

extern "C" {

DllExport LPSTR __stdcall enumeratePorts();
DllExport WORD __stdcall  open(LPSTR port);
DllExport void __stdcall reportAnalog(WORD channel, WORD enable);
DllExport void __stdcall reportDigital(WORD port, WORD enable);
DllExport void __stdcall pinMode(WORD pin, WORD mode);
DllExport void __stdcall servo(WORD pin);
DllExport void __stdcall digitalWrite(WORD pin, WORD value);
DllExport void __stdcall analogWrite(WORD pin, DWORD value);
DllExport void __stdcall setSamplingInterval(DWORD value);
DllExport void __stdcall talkTo(WORD value);
DllExport WORD __stdcall digitalRead(WORD pin);
DllExport DWORD __stdcall analogRead(LPSTR channel);
DllExport void __stdcall configI2C(DWORD value);
DllExport void __stdcall reportI2C(WORD address, WORD reg, DWORD bytes);
DllExport void __stdcall writeI2C(WORD address, PCSTR data);
DllExport DWORD __stdcall readI2C(WORD address, WORD reg, PBYTE Buffer);
DllExport DWORD __stdcall readI2COnce(WORD address, WORD reg, DWORD bytes, PBYTE Buffer);

DllExport uint8_t __stdcall I2CLoad(PCSTR libname);
DllExport void __stdcall I2CCommand(uint8_t _index, PCSTR cmd, PCSTR data);
DllExport void __stdcall I2CwriteStr(uint8_t address, PCSTR data);

}

std::vector<interfaz::Interfaz> i;
int index = -1;

extern "C" BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		i.reserve(8);
	break;
    case DLL_THREAD_ATTACH:
		//MessageBox(NULL, "DLL_THREAD_ATTACH", "INTERFAZ", 0);
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH:
		//MessageBox(NULL, "Se perdió la conexión con DLL", "INTERFAZ", 0);
		for (std::vector<interfaz::Interfaz>::iterator it = i.begin(); it != i.end(); ++it) {
			(*it).serialio->close();
		}
        break;
    }
    return TRUE;
}

std::vector<uint8_t> PCSTR2ByteVector(PCSTR data) {
	std::string s(data);
	std::vector<uint8_t> v;
	if (s.size()) {
		std::istringstream is(s);
		std::string n;
		while (is >> n) {
			v.push_back(std::stoi(n, nullptr, 0));
		}
	}
	return v;
}

uint8_t* PCSTR2ByteArray(PCSTR data) {
	return PCSTR2ByteVector(data).data();
}


void reconnect(exception e) {
	cout << e.what() << endl;
	i.at(index).serialio->close();
	i.at(index).serialio->open();
}

LPSTR __stdcall enumeratePorts() {
	std::vector<firmata::PortInfo> ports = firmata::FirmSerial::listPorts();
	std::string res("");
	for (auto port : ports) {
		res += port.port + ": " + port.description + '\n';
	}
	char * str = static_cast<char*>(GlobalAlloc(GPTR, res.size() + 1));
	strcpy_s(str, res.size() + 1, res.c_str());

	return str;
}

WORD __stdcall open(LPSTR port) {
	try {
		interfaz::Interfaz j(port);
		i.push_back(j);
		index++; 
		std::thread{ &interfaz::Interfaz::parse, j }.detach();

		i.at(index).f->clearLCD();
		string a("Conectado: ");
		string p(port);
		a.append(p);
		i.at(index).f->pushLCD(a.c_str());
		string b("Soy interfaz: ");
		b.append(std::to_string(index+1));
		i.at(index).f->pushLCD(b.c_str());

		return index + 1;
	} 
	catch (exception e) {
		cout << e.what() << endl;
		return 0;
	}
}

void __stdcall reportAnalog(WORD channel, WORD enable) {
	try {
		i.at(index).f->reportAnalog((uint8_t)channel, (uint8_t)enable);
	}
	catch (exception e) {
		reconnect(e);
		reportAnalog(channel, enable);
	}
}

void __stdcall reportDigital(WORD port, WORD enable) {
	try {
		i.at(index).f->reportDigital((uint8_t)port, (uint8_t)enable);
	}
	catch (exception e) {
		reconnect(e);
		reportDigital(port, enable);
	}
}

void __stdcall setSamplingInterval(DWORD value) {
	try {
		i.at(index).f->setSamplingInterval(value);
	}
	catch (exception e) {
		reconnect(e);
		setSamplingInterval(value);
	}
}

void __stdcall talkTo(WORD value) {
	if ((value > 0) && (value <= i.size())) {
		index = value - 1;
	}
}

void __stdcall servo(WORD pin) {
	try {
		i.at(index).f->pinMode((uint8_t)pin, FIRMATA_PIN_MODE_SERVO);
	}
	catch (exception e) {
		reconnect(e);
		servo(pin);
	}
}

void __stdcall pinMode(WORD pin, WORD mode) {
	try {
		i.at(index).f->pinMode((uint8_t)pin, (uint8_t)mode);
	}
	catch (exception e) {
		reconnect(e);
		pinMode(pin, mode);
	}
}

void __stdcall digitalWrite(WORD pin, WORD value) {
	try {
		i.at(index).f->digitalWrite((uint8_t)pin, (uint8_t)value);
	}
	catch (exception e) {
		reconnect(e);
		digitalWrite(pin, value);
	}
}

void __stdcall analogWrite(WORD pin, DWORD value) {
	try {
		i.at(index).f->analogWrite((uint8_t)pin, (uint32_t)value);
	}
	catch (exception e) {
		reconnect(e);
		analogWrite(pin, value);
	}
}

WORD __stdcall digitalRead(WORD pin) {
	return (WORD)i.at(index).f->digitalRead((uint8_t)pin);
}

DWORD __stdcall analogRead(LPSTR channel) {
	return (DWORD)i.at(index).f->analogRead(channel);
}

void __stdcall configI2C(DWORD value) {
	i.at(index).f->configI2C((uint16_t)value);
}

void __stdcall reportI2C(WORD address, WORD reg, DWORD bytes) {
	i.at(index).f->reportI2C((uint16_t)address, (uint16_t)reg, (uint32_t)bytes);
}

void __stdcall writeI2C(WORD address, PCSTR data) {
	std::vector<uint8_t> v = PCSTR2ByteVector(data);
	i.at(index).f->writeI2C((uint16_t)address, v);
}

DWORD __stdcall readI2C(WORD address, WORD reg, PBYTE Buffer) {
	std::vector<uint8_t> data = i.at(index).f->readI2C((uint16_t)address, (uint16_t)reg);
	memcpy(Buffer, data.data(), data.size());
	return data.size();
}

DWORD __stdcall readI2COnce(WORD address, WORD reg, DWORD bytes, PBYTE Buffer) {
	std::vector<uint8_t> data = i.at(index).f->readI2COnce((uint16_t)address, (uint16_t)reg, (uint32_t)bytes);
	memcpy(Buffer, data.data(), bytes);
	return data.size();
}

uint8_t __stdcall I2CLoad(PCSTR libname) {
	return i.at(index).I2CLoad(libname);
}

void __stdcall I2CCommand(uint8_t _index, PCSTR cmd, PCSTR data) {
	i.at(index).I2CCommand(_index, cmd, PCSTR2ByteVector(data).data());

}

void __stdcall I2CwriteStr(uint8_t _index, PCSTR data) {
	i.at(index).I2CwriteStr(_index, data);

}
