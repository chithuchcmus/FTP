#pragma once
#define eom EndOfMessage
#define gc GetCode
#define MAX_BUFF 1024
#include "stdafx.h"

int EndOfMessage(char*);
char* GetCode(char*, char*);
void PortConvert(unsigned short, unsigned char &, unsigned char &);
void GetLocalAddress(SOCKET&, unsigned char&, unsigned char&, unsigned char&, unsigned char&);
std::string GetFileName(std::string s);
bool IsAbsolutePath(std::string s);

class Program {
private:
	SOCKET command;
	SOCKET data;
	std::string current_dir;
	sockaddr_in sv_address;
	unsigned short dataport;
	bool in_passive;
public:
	int (Program::*f)();
	Program(const SOCKET &c, sockaddr_in &sv);
	Program(const SOCKET &c, sockaddr_in &sv, std::string dir);
	int Menu();
	int Send(SOCKET& sock, std::string s, int flag);
	int Recv(SOCKET& sock, std::string s, int flag);
	int Login();
	int Quit();
	int List();
	int Port();
	int Store();
	int mStore();
	int Retrieve();
	int mRetrieve();
	int Cwd();
	int Lcd();
	int Dele();
	int mDele();
	int MakeDir();
	int PrintWorkingDir();
	int RemoveDir();
	int PassivePort(unsigned int& port);
	int SwitchModes();
	int OpenDataConnection(bool forListen);
	int ActivateDataConnection();
};