#include "stdafx.h"
using namespace std;


//Thêm \0 vào cuối chuỗi, nếu chuỗi kết thúc bằng \r\n
int EndOfMessage(char* s) {
	int i = 0;
	while ((s[0] != '\r' || s[1] != '\n') && i < 512 - 2)
		s++, i++;

	if (i < 512 - 2) {
		s[2] = '\0';
		return 0;
	}

	if (s[0] == '\r' && s[1] == '\n') {
		s[0] = '\n';
		s[1] = '\0';
		return 0;
	}

	if (s[1] == '\r')
		return 1;

	return 2;
}

//Lấy mã 
char* GetCode(char* message, char* code) {
	char* tmp = code;
	int i = 0;

	while (message[0] != ' ' && i < MAX_BUFF) {
		if (message[0] < '0' || message[0] > '9') {
			memset(code, 0, 10 - i);
			return NULL;
		}

		code[0] = message[0];
		code++; message++;
		i++;
	}
	code[0] = '\0';
	return tmp;
}

//Chuyển số port (2^16) thành 2 số (2^8)
void PortConvert(unsigned short p, unsigned char &a, unsigned char &b) {
	int tmp[16] = { 0 };
	int i = 0;
	a = b = 0;

	while (i < 16) {
		tmp[i] = p % 2;
		p /= 2;

		if (i < 8) {
			tmp[i] <<= i;
			b += tmp[i];
		}
		else {
			tmp[i] <<= i - 8;
			a += tmp[i];
		}

		i++;
	}
}

void GetLocalAddress(SOCKET& sock, unsigned char& a, unsigned char& b, unsigned char& c, unsigned char& d) {
	sockaddr_in my_addr;
	int addrlen = sizeof(my_addr);
	getsockname(sock, (sockaddr*)&my_addr, &addrlen);
	a = my_addr.sin_addr.S_un.S_un_b.s_b1;
	b = my_addr.sin_addr.S_un.S_un_b.s_b2;
	c = my_addr.sin_addr.S_un.S_un_b.s_b3;
	d = my_addr.sin_addr.S_un.S_un_b.s_b4;
}

//Kiểm tra đường dẫn là tuyệt đối hay tương đối 
//(nếu là tương đối thì phải gắn thêm đường dẫn hiện hành vào)
//Đường dẫn tuyệt đối phải bắt đầu bằng tên ổ đĩa (?) vd: C:\a\b\c\d.txt
bool IsAbsolutePath(string s)
{
	int len = s.size();
	//Bỏ dấu ngoặc kép nếu có
	if (s[0] == '\"' && s[len - 1] == '\"') {
		s = s.substr(1);
		s[len - 1] = 0;
		s.pop_back();
	}

	if (isalpha(s[0]) && s[1] == ':')
		return true;
	return false;
}

string GetFileName(string path) {
	string ans;
	int i;
	int len = path.size();
	//Lấy tên file
	//Nếu địa chỉ có dấu ngoặc kép thì bỏ
	if (path[len - 1] == '\"' && path[0] == '\"') {
		path[len - 1] = path[0] = '\0';
		path.pop_back();
		path = path.substr(1);
		len -= 2;
	}

	//Tìm dấu \ đầu tiên từ phải qua rồi cắt lấy tên file
	for (i = len - 1; i >= 0; i--)
		if (path[i] == '\\')
			break;

	ans = path.substr(i + 1);
	return ans;
}

Program::Program(const SOCKET& c, sockaddr_in &sv) {
	command = c;
	data = 0;
	sv_address = sv;
	current_dir = "";
	in_passive = false;
}

Program::Program(const SOCKET& c, sockaddr_in &sv, string s) {
	command = c;
	data = 0;
	sv_address = sv;
	current_dir = s;
	in_passive = false;
}

int Program::Menu() {
	int nResult;
	int i = 0;
	bool showMenu = true;
	CMenu menu(this);

	//Thêm các tùy chọn cho menu
	menu.Add("List", &Program::List);
	menu.Add("Get", &Program::Retrieve);
	menu.Add("mGet", &Program::mRetrieve);

	menu.Add("put", &Program::Store);
	menu.Add("mPut", &Program::mStore);
	menu.Add("Cd", &Program::Cwd);
	menu.Add("Lcd", &Program::Lcd);
	menu.Add("Delete", &Program::Dele);
	menu.Add("mDelete", &Program::mDele);
	
	menu.Add("Mkdir", &Program::MakeDir);
	menu.Add("Rmdir", &Program::RemoveDir);
	menu.Add("Pwd", &Program::PrintWorkingDir);
	menu.Add("Switch Modes", &Program::SwitchModes);
	
login:
	nResult = Login();
	if (nResult != 230) {
		cout << "Ban muon login lai? (1/0)";
		cin >> nResult;
		if (nResult)
			goto login;
		else
			goto quit;
	}

	//Hiện menu, yêu cầu làm
	
	do {
		if (showMenu) {
			menu.Show();
			showMenu = false;
		}
		i = menu.Select();
		cout << endl;
		if (i == 400)
			showMenu = true;
	} while (i != 404);

quit:
	cout << "Ket thuc ket noi voi server\n";
	Quit();
	return nResult;
}

//Gửi qua socket tin s hoặc nội dung file có đường dẫn là s
//Flag = 0: tin. Flag = 1: file
int Program::Send(SOCKET& sock, string s, int flag) 
{
	if (flag == 0) {
		send(sock, s.c_str(), s.length(), 0);
	}
	else
	{
		if (!IsAbsolutePath(s))
			s = current_dir + s;

		if (s[0] == '\"' && s[s.size() - 1] == '\"')
			s = s.substr(1, s.size() - 2);
		FILE *f = fopen(s.c_str(), "rb");
		if (f == NULL)
		{
			cout << "Ban Nhap Duong Dan Sai,File khong ton tai " << endl;
			return 0;
		}
		char buf[MAX_BUFF + 1];
		int len = 0;

		memset(buf, 0, MAX_BUFF + 1);

		while (len = fread(buf, 1, MAX_BUFF, f)) 
		{
			send(sock, buf, len, 0);
			memset(buf, 0, MAX_BUFF + 1);
		}

		fclose(f);
	}
	return 0;
}

//Nhận từ socket nội dung file có tên s
//Flag = 0: tin (không cần s). Flag = 1: tên file
int Program::Recv(SOCKET& sock, string s, int flag) {
	char buf[MAX_BUFF + 1];
	char Code[10];
	int byteRcv = 0;


	FILE *f = NULL;
	if (flag == 1) {
		if (!IsAbsolutePath(s))
			s = current_dir + s;
		f = fopen(s.c_str(), "wb");
		if (f == NULL)
			return -1;
	}
	memset(buf, 0, MAX_BUFF + 1);
	byteRcv = recv(sock, buf, MAX_BUFF, 0);
	if (byteRcv > 0)
		gc(buf, Code);

	//Ghi vào file hoặc in ra màn hình
	if (flag)
		fwrite(buf, byteRcv, 1, f);
	else
		cout << buf;

	while (buf[byteRcv - 2] != '\r' || buf[byteRcv - 1] != '\n') {
		memset(buf, 0, MAX_BUFF + 1);
		byteRcv = recv(sock, buf, MAX_BUFF, 0);

		if (byteRcv <= 0)
			break;

		if (flag)
			fwrite(buf, byteRcv, 1, f);
		else
			cout << buf;
	}
	if (f)
		fclose(f);

	return atoi(Code);
}

int Program::Login() {
	string user;
	string pass;
	string msg;

	cout << "User: ";
	cin >> user;

	msg = "USER " + user + "\r\n";
	Send(command, msg, 0);
	Recv(command, "", 0);

	cout << "Password: ";
	cin >> pass;

	msg = "PASS " + pass + "\r\n";
	Send(command, msg, 0);
	return Recv(command, "", 0);
}

int Program::Quit() {
	Send(command, "QUIT\r\n", 0);
	return 0;
}

int Program::List()
{
	string filename;
	cout << "Nhap ten folder muon xem: ";
	cin >> filename;
	cin.ignore();

	char detailed = 'Y';
	do {
		cout << "Xem chi tiet ? (Y/N): ";
		cin >> detailed;
		detailed = toupper(detailed);
	} while (detailed != 'Y' && detailed != 'N');

	string msg = detailed == 'Y' ? "LIST " : "NLST ";
	msg += filename + "\r\n";

	//Mở kết nối data
	OpenDataConnection(true);

	Recv(command, "", 0);
	Send(command, msg, 0);
	Recv(command, "", 0);

	if (in_passive) {
		Recv(data, "", 0);
	}
	else {
		SOCKET s = accept(data, NULL, NULL);
		Recv(s, "", 0);
		closesocket(s);
	}
	
	closesocket(data);

	return Recv(command, "", 0);
}

int Program::Port() {
	unsigned short port;
	unsigned char a, b;
	unsigned char c, d, e, f;
	char tmp[100];
	string msg;

	port = rand() % PORT_RANGE + 1024;

	PortConvert(port, a, b);
	GetLocalAddress(command, c, d, e, f);
	dataport = port;

	sprintf(tmp, "PORT %u,%u,%u,%u,%u,%u\r\n", c, d, e, f, a, b);
	msg = tmp;

	Send(command, msg, 0);
	return Recv(command, "", 0);
}

int Program::PassivePort(unsigned int& port) {
	unsigned int a, b;
	unsigned int c, d, e, f;

	string msg = "PASV\r\n";
	Send(command, msg, 0);
	char buffer[MAX_BUFF + 1];
	char Code[10] = { 0 };
	unsigned int byteRecv = recv(command, buffer, MAX_BUFF, 0);
	buffer[byteRecv] = '\0';
	cout << buffer;
	gc(buffer, Code);

	int start = byteRecv - 1;
	while (start >= 0 && buffer[start] != '(') {
		start--;
	}
	int end = start + 1;
	while (end < byteRecv && buffer[end] != ')') {
		end++;
	}

	if (end == byteRecv || start < 0) {
		return atoi(Code);
	}
	
	char hostnport[MAX_BUFF + 1] = { 0 };
	for (int i = start + 1; i < end; i++) {
		hostnport[i - start - 1] = buffer[i];
	}

	sscanf(hostnport, "%u,%u,%u,%u,%u,%u", &c, &d, &e, &f, &a, &b);

	port = a * 16 * 16 + b;

	dataport = rand() % PORT_RANGE + 1024;
	return atoi(Code);
}

int Program::Store() {
	string filename;
	string msg;

	cin.ignore();
	cout << "Nhap ten file: ";
	getline(cin, filename);
	
	OpenDataConnection(false);

	msg = "STOR " + GetFileName(filename) + "\r\n";
	Send(command, msg, 0);
	Recv(command, "", 0);

	if (!in_passive) {
		ActivateDataConnection();
	}
	
	Send(data, filename, 1);

	closesocket(data);

	return Recv(command, "", 0);	
}
//int  Program::mStore()
//{
//	int countFile;
//	string filename;
//	string msg;
//	sockaddr_in my_addr;
//	cout << "Nhap so file can gui: " << endl;
//	cin >> countFile;
//
//	
//
//	cin.ignore();
//	for (int i = 1; i <= countFile; i++)
//	{
//		Port();
//		Recv(command, "", 0);
//		Gắn socket vào port "dataport"
//
//		my_addr.sin_port = htons(dataport);
//		data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//		SetupIPWS(data, my_addr, false);
//
//		sockaddr_in sv_data_addr;
//		sv_data_addr = sv_address;
//		sv_data_addr.sin_port = htons(SERVER_D_PORT);
//		khi chay cai nay no bi doi dia chi sever
//		char c_serveraddr[50] = "192.168.10.5";
//		sv_data_addr.sin_addr.s_addr = inet_addr(c_serveraddr);
//
//
//		cout << "Nhap ten file: ";
//		getline(cin, filename);
//		msg = "STOR " + GetFileName(filename) + "\r\n";
//		Send(command, msg, 0);
//		Recv(command, "", 0);
//		if (Connect(data, sv_data_addr))
//		{
//			cout << "ket noi that bai" << endl;
//		}
//		else
//			cout << "ket noi thanh cong" << endl;
//		Send(data, filename, 1);
//		Recv(command, "", 0);
//		closesocket(data);
//	}
//
//	return 0;
//}
int  Program::mStore()
{
	int countFile;
	string filename;
	string msg;
	do
	{	
		cout << "Nhap so file can gui(nhap So): " << endl;
		cin >> countFile;
		if (countFile < 0)
		{
			cout << "Ban nhap So File  < 0 , vui long nhap lai" << endl;
		}
	} while (countFile < 0 );

	cin.ignore();
	for (int i = 1; i <= countFile; i++)
	{
		//// Check server address
		//cout << (unsigned int)sv_address.sin_addr.S_un.S_un_b.s_b1 << "."
		//	 << (unsigned int)sv_address.sin_addr.S_un.S_un_b.s_b2 << "."
		//	 << (unsigned int)sv_address.sin_addr.S_un.S_un_b.s_b3 << "."
		//	 << (unsigned int)sv_address.sin_addr.S_un.S_un_b.s_b4 << "\n";

		cout << "Nhap ten file: ";
		getline(cin, filename);

		//Mở kết nối data
		OpenDataConnection(false);

		msg = "STOR " + GetFileName(filename) + "\r\n";
		Send(command, msg, 0);
		Recv(command, "", 0);

		if (!in_passive) {
			ActivateDataConnection();
		}

		Send(data, filename, 1);

		closesocket(data);
		Recv(command, "", 0);
	}

	return Recv(command, "", 0);
}

int Program::Retrieve() {
	string filename;
	string msg;

	cin.ignore();
	cout << "Nhap ten file: ";
	getline(cin, filename);

	OpenDataConnection(true);

	msg = "RETR " + filename + "\r\n";
	Send(command, msg, 0);
	Recv(command, "", 0);

	if (in_passive) {
		Recv(data, filename, 1);
	}
	else {
		SOCKET s = accept(data, NULL, NULL);
		Recv(s, filename, 1);
		closesocket(s);
	}

	closesocket(data);
	Recv(command, "", 0);
	return 0;
}
int Program::mRetrieve()
{
	int countFile;
	string filename;
	string msg;
	do
	{
		cout << "Nhap so file can Nhan(nhap So): " << endl;
		cin >> countFile;
		if (countFile < 0)
		{
			cout << "Ban nhap So File  <= 0 , vui long nhap lai" << endl;
		}
	} while (countFile <= 0);

	cin.ignore();
	for (int i = 1; i <= countFile; i++)
	{
		cout << "Nhap ten file: ";
		getline(cin, filename);
	
		OpenDataConnection(true);
		
		msg = "RETR " + GetFileName(filename) + "\r\n";
		Send(command, msg, 0);
		Recv(command, "", 0);
	
		if (in_passive) {
			Recv(data, filename, 1);
		}
		else {
			SOCKET s = accept(data, NULL, NULL);
			Recv(s, filename, 1);
			closesocket(s);
		}

		closesocket(data);
		Recv(command, "", 0);
	}

	Recv(command, "", 0);
	return 0;
}
int Program::Cwd()
{
	string filename;
	string msg;

	cin.ignore();
	cout << "Nhap ten file: ";
	getline(cin, filename);

	////Yêu cầu server gửi data trên port "dataport"
	//Port();
	//Recv(command, "", 0);

	////Gắn socket vào port "dataport"
	//my_addr.sin_port = htons(dataport);
	//data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//SetupIPWS(data, my_addr, false);


	msg = "CWD " + GetFileName(filename) + "\r\n";
	Send(command, msg, 0);
	Recv(command, "", 0);
	//closesocket(data);
	return 0;
}

int Program::Lcd()
{
	string path;
	cout << "Nhap Duong dan can thay doi duoi client: " << endl;
	cin >> path;
	current_dir = path;
	cout << "Successful " << endl;
	return 0;
}
int  Program::Dele()
{
	//data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//sockaddr_in my_addr;

	////Yêu cầu server gửi data trên port "dataport"
	//Port();
	//Recv(command, "", 0);

	////Gắn socket lên ip chính mình và port "dataport"
	//my_addr.sin_port = htons(dataport);
	//SetupIPWS(data, my_addr, false);

	string filename;
	string msg;

	cin.ignore();
	cout << "Nhap ten file can XOA: ";
	getline(cin, filename);
	msg = "DELE " + filename + "\r\n";
	Send(command, msg, 0);
	Recv(command, "", 0);
	//closesocket(data);
	return 0;
}

int Program::mDele()
{
	int countFile;
	string filename;
	string msg;
	//sockaddr_in my_addr;
	do
	{
		cout << "Nhap so file can Xoa(nhap So): " << endl;
		cin >> countFile;
		if (countFile < 0)
		{
			cout << "Ban nhap So File  <= 0 , vui long nhap lai" << endl;
		}
	} while (countFile <= 0);
	cin.ignore();

	for (int i = 1; i <= countFile; i++)
	{
		//data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		////Yêu cầu server gửi data trên port "dataport"
		//Port();
		//Recv(command, "", 0);

		////Gắn socket lên ip chính mình và port "dataport"
		//my_addr.sin_port = htons(dataport);
		//SetupIPWS(data, my_addr, false);

		cout << "Nhap ten file can XOA: ";
		getline(cin, filename);
		msg = "DELE " + filename + "\r\n";
		Send(command, msg, 0);
		Recv(command, "", 0);
		//closesocket(data);
	}
	return 0;
}

int Program::MakeDir() {
	string dirName;
	cout << "Nhap ten thu muc muon tao: ";
	cin >> dirName;

	string msg = "MKD " + dirName + "\r\n";

	Send(command, msg, 0);
	return Recv(command, "", 0);
}

int Program::RemoveDir() {
	string filename;
	cout << "Nhap ten folder muon xoa: ";
	cin >> filename;

	string msg = "RMD " + filename + "\r\n";
	Send(command, msg, 0);
	return Recv(command, "", 0);
}

int Program::PrintWorkingDir() {
	string msg = "PWD\r\n";

	Send(command, msg, 0);
	return Recv(command, "", 0);
}

int Program::SwitchModes() {
	if (in_passive) {
		cout << "Active Mode On\n";
	}
	else {
		cout << "Passive Mode On\n";
	}
	return in_passive = !in_passive;
}

int Program::OpenDataConnection(bool forListen) {
	sockaddr_in my_addr;
	unsigned int port;
	if (!in_passive) {
		Port();
	}
	else {
		PassivePort(port);
	}

	//Gắn socket lên ip chính mình và port "dataport"
	my_addr.sin_port = htons(dataport);
	data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SetupIPWS(data, my_addr, false);

	if (in_passive) {
		sockaddr_in sv_data_addr;
		sv_data_addr = sv_address;
		sv_data_addr.sin_port = htons(port);

		return Connect(data, sv_data_addr);
	}
	else if (forListen) {
		if (Recv(command, "", 0) != 550) {
			//Chờ kết nối của server đến socket data
			if (listen(data, SOMAXCONN) == SOCKET_ERROR) {
				printf("Listen failed with error: %ld\n", WSAGetLastError());
				closesocket(data);
				WSACleanup();
				return 1;
			}
			return 0;
		}
		else {
			return 1;
		}
	}
	return 0;
}

int Program::ActivateDataConnection() {
	sockaddr_in sv_data_addr;
	sv_data_addr = sv_address;
	sv_data_addr.sin_port = htons(SERVER_D_PORT);

	return Connect(data, sv_data_addr);
}