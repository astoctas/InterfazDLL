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

// Arduino
DllExport void __stdcall enumeratePorts(PBYTE Buffer);
DllExport WORD __stdcall  open(LPSTR port);
DllExport void __stdcall  close();
DllExport void __stdcall reportAnalog(WORD channel, WORD enable);
DllExport void __stdcall reportDigital(WORD port, WORD enable);
DllExport void __stdcall pinMode(WORD pin, WORD mode);
DllExport void __stdcall servo(WORD pin);
DllExport void __stdcall digitalWrite(WORD pin, WORD value);
DllExport void __stdcall analogWrite(WORD pin, DWORD value);
DllExport void __stdcall setSamplingInterval(DWORD value);
DllExport WORD __stdcall digitalRead(WORD pin);
DllExport DWORD __stdcall analogRead(LPSTR channel);
DllExport void __stdcall configI2C(DWORD value);
DllExport void __stdcall reportI2C(WORD address, WORD reg, DWORD bytes);
DllExport void __stdcall writeI2C(WORD address, PCSTR data);
DllExport DWORD __stdcall readI2C(WORD address, WORD reg, PBYTE Buffer);
DllExport DWORD __stdcall readI2COnce(WORD address, WORD reg, DWORD bytes, PBYTE Buffer);

// Interfaz
DllExport void __stdcall talkTo(WORD value);
DllExport void __stdcall outputs(PCSTR data);
DllExport void __stdcall outputsOn();
DllExport void __stdcall outputsOff();
DllExport void __stdcall outputsReverse();
DllExport void __stdcall outputsBrake();
DllExport void __stdcall outputsDir(uint8_t dir);
DllExport void __stdcall outputsSpeed(uint8_t speed);
DllExport void __stdcall steppers(PCSTR data);
DllExport void __stdcall steppersSteps(int32_t steps);
DllExport void __stdcall steppersStop();
DllExport void __stdcall steppersDir(uint8_t dir);
DllExport WORD __stdcall steppersStatus(PBYTE Buffer);

DllExport void __stdcall servos(PCSTR data);
DllExport void __stdcall servosPosition(uint8_t pos);
DllExport void __stdcall analogOn(uint8_t channel);
DllExport void __stdcall analogOff(uint8_t channel);
DllExport uint16_t __stdcall analogValue();
DllExport void __stdcall i2c(uint8_t address, uint32_t delay);
DllExport void __stdcall i2cReport(uint16_t _register, uint32_t bytes);
DllExport uint16_t __stdcall i2cRead(uint16_t _register, uint32_t bytes, PBYTE Buffer);
DllExport uint16_t __stdcall i2cValue(uint16_t _register, PBYTE Buffer);
DllExport void __stdcall i2cWrite(PCSTR data);
DllExport void __stdcall lcdWrite(uint8_t row, PCSTR data);
DllExport void __stdcall lcdClear();

// I2C Plugins
DllExport uint8_t __stdcall I2CLoad(PCSTR libname);
DllExport void __stdcall I2CCommand(uint8_t _index, PCSTR cmd, PCSTR data);
DllExport void __stdcall I2CwriteStr(uint8_t address, PCSTR data);

}

std::vector<interfaz::Interfaz*> i;
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
		for (std::vector<interfaz::Interfaz*>::iterator it = i.begin(); it != i.end(); ++it) {
			(*it)->f->clearLCD();
			(*it)->f->printLCD(0, "Desconectado");
			(*it)->serialio->close();
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


void reconnect(exception e) {
	cout << e.what() << endl;
	i.at(index)->serialio->close();
	i.at(index)->serialio->open();
}

void __stdcall enumeratePorts(PBYTE Buffer) {
	std::vector<firmata::PortInfo> ports = firmata::FirmSerial::listPorts();
	std::string res("");
	for (auto port : ports) {
		res += port.port + ": " + port.description + '\n';
	}
	memcpy(Buffer, res.data(), res.size());
}

WORD __stdcall open(LPSTR port) {
	try {
		interfaz::Interfaz* j = static_cast<interfaz::Interfaz*>(GlobalAlloc(GPTR, sizeof(interfaz::Interfaz)));
		index++;
		j = new interfaz::Interfaz(port, index);
		i.push_back(j);
		std::thread{ &interfaz::Interfaz::parse, j }.detach();
		/* TODO FLUSH BUFFERS */

		return index + 1;
	} 
	catch (exception e) {
		cout << e.what() << endl;
		return 0;
	}
}

void __stdcall close() {
	i.at(index)->close();
	index--;
}

void __stdcall reportAnalog(WORD channel, WORD enable) {
	try {
		i.at(index)->f->reportAnalog((uint8_t)channel, (uint8_t)enable);
	}
	catch (exception e) {
		reconnect(e);
		reportAnalog(channel, enable);
	}
}

void __stdcall reportDigital(WORD port, WORD enable) {
	try {
		i.at(index)->f->reportDigital((uint8_t)port, (uint8_t)enable);
	}
	catch (exception e) {
		reconnect(e);
		reportDigital(port, enable);
	}
}

void __stdcall setSamplingInterval(DWORD value) {
	try {
		i.at(index)->f->setSamplingInterval(value);
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
		i.at(index)->f->pinMode((uint8_t)pin, FIRMATA_PIN_MODE_SERVO);
	}
	catch (exception e) {
		reconnect(e);
		servo(pin);
	}
}

void __stdcall pinMode(WORD pin, WORD mode) {
	try {
		i.at(index)->f->pinMode((uint8_t)pin, (uint8_t)mode);
	}
	catch (exception e) {
		reconnect(e);
		pinMode(pin, mode);
	}
}

void __stdcall digitalWrite(WORD pin, WORD value) {
	try {
		i.at(index)->f->digitalWrite((uint8_t)pin, (uint8_t)value);
	}
	catch (exception e) {
		reconnect(e);
		digitalWrite(pin, value);
	}
}

void __stdcall analogWrite(WORD pin, DWORD value) {
	try {
		i.at(index)->f->analogWrite((uint8_t)pin, (uint32_t)value);
	}
	catch (exception e) {
		reconnect(e);
		analogWrite(pin, value);
	}
}

WORD __stdcall digitalRead(WORD pin) {
	return (WORD)i.at(index)->f->digitalRead((uint8_t)pin);
}

DWORD __stdcall analogRead(LPSTR channel) {
	return (DWORD)i.at(index)->f->analogRead(channel);
}

void __stdcall configI2C(DWORD value) {
	i.at(index)->f->configI2C((uint16_t)value);
}

void __stdcall reportI2C(WORD address, WORD reg, DWORD bytes) {
	i.at(index)->f->reportI2C((uint16_t)address, (uint16_t)reg, (uint32_t)bytes);
}

void __stdcall writeI2C(WORD address, PCSTR data) {
	std::vector<uint8_t> v = PCSTR2ByteVector(data);
	i.at(index)->f->writeI2C((uint16_t)address, v);
}

DWORD __stdcall readI2C(WORD address, WORD reg, PBYTE Buffer) {
	std::vector<uint8_t> data = i.at(index)->f->readI2C((uint16_t)address, (uint16_t)reg);
	memcpy(Buffer, data.data(), data.size());
	return data.size();
}

DWORD __stdcall readI2COnce(WORD address, WORD reg, DWORD bytes, PBYTE Buffer) {
	std::vector<uint8_t> data = i.at(index)->f->readI2COnce((uint16_t)address, (uint16_t)reg, (uint32_t)bytes);
	memcpy(Buffer, data.data(), bytes);
	return data.size();
}

uint8_t __stdcall I2CLoad(PCSTR libname) {
	return i.at(index)->I2CLoad(libname);
}

void __stdcall I2CCommand(uint8_t _index, PCSTR cmd, PCSTR data) {
	i.at(index)->I2CCommand(_index, cmd, PCSTR2ByteVector(data).data());

}

void __stdcall I2CwriteStr(uint8_t _index, PCSTR data) {
	i.at(index)->I2CwriteStr(_index, data);
}

void __stdcall outputs(PCSTR data) {
	std::vector<uint8_t> v = PCSTR2ByteVector(data);
	i.at(index)->setOutputs(v);
}

void __stdcall outputsOn() {
	i.at(index)->outputsOn();
}

void __stdcall outputsOff() {
	i.at(index)->outputsOff();
}

void __stdcall outputsReverse() {
	i.at(index)->outputsReverse();
}

void __stdcall outputsBrake() {
	i.at(index)->outputsBrake();
}

void __stdcall outputsDir(uint8_t dir) {
	i.at(index)->outputsDir(dir);
}

void __stdcall outputsSpeed(uint8_t speed) {
	i.at(index)->outputsSpeed(speed);
}

void __stdcall steppers(PCSTR data) {
	std::vector<uint8_t> v = PCSTR2ByteVector(data);
	i.at(index)->setSteppers(v);
}

void __stdcall steppersSteps(int32_t steps) {
	i.at(index)->steppersSteps(steps);
}

void __stdcall steppersStop() {
	i.at(index)->steppersStop();
}

void __stdcall steppersDir(uint8_t dir) {
	i.at(index)->steppersDir(dir);
}

WORD __stdcall steppersStatus(PBYTE Buffer) {
	std::vector<uint8_t> data = i.at(index)->steppersStatus();
	memcpy(Buffer, data.data(), data.size());
	return data.size();
}

void __stdcall servos(PCSTR data) {
	std::vector<uint8_t> v = PCSTR2ByteVector(data);
	i.at(index)->setServos(v);
}

void __stdcall servosPosition(uint8_t pos) {
	i.at(index)->servosPosition(pos);
}

void __stdcall analogOn(uint8_t channel) {
	i.at(index)->setAnalog(channel, 1);
}

void __stdcall analogOff(uint8_t channel) {
	i.at(index)->setAnalog(channel, 0);
}

uint16_t __stdcall analogValue() {
	return i.at(index)->analogValue();
}


void __stdcall i2c(uint8_t address, uint32_t delay) {
	i.at(index)->setI2C(address, delay);
};

void __stdcall i2cReport(uint16_t _register, uint32_t bytes) {
	i.at(index)->i2cReport(_register, bytes);
};

uint16_t __stdcall i2cRead(uint16_t _register, uint32_t bytes, PBYTE Buffer) {
	std::vector<uint8_t> data = i.at(index)->i2cRead(_register, bytes);
	memcpy(Buffer, data.data(), bytes);
	return data.size();
};

uint16_t __stdcall i2cValue(uint16_t _register, PBYTE Buffer) {
	std::vector<uint8_t> data = i.at(index)->i2cValue(_register);
	memcpy(Buffer, data.data(), data.size());
	return data.size();

};

void __stdcall i2cWrite(PCSTR data) {
	std::vector<uint8_t> v = PCSTR2ByteVector(data);
	i.at(index)->i2cWrite(v);
};

void __stdcall lcdWrite(uint8_t row, PCSTR data) {
	i.at(index)->f->printLCD(row, data);
}

void __stdcall lcdClear() {
	i.at(index)->f->clearLCD();
}


/* IMPRIMIR

char buf[10] = "";
_itoa_s(deviceNum, buf, 10);
i->f->pushLCD(buf);

*/