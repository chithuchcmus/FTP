#include "stdafx.h"
//using namespace std;


int SetupIPWS(SOCKET& Sock, sockaddr_in& MyIP, bool randomport)
{

	srand(time(NULL));

	MyIP.sin_family = AF_INET;
	if (randomport)
		MyIP.sin_port = htons(rand() % PORT_RANGE + 1024);
	MyIP.sin_addr.s_addr = INADDR_ANY;


	int nResult = bind(Sock, (struct sockaddr*)&MyIP, sizeof(struct sockaddr));
	if (nResult) {
		std::cout << "Loi khi thiet lap" << std::endl;
		WSAGetLastError();
		closesocket(Sock);
		return 1;
	}

	return 0;
}

int Connect(SOCKET& Sock, sockaddr_in& server) {
	int try_count = 0;
	bool error_flag = true;
	while (try_count < ERROR_LIMIT)
	{
		int nResult = connect(Sock, (sockaddr*)&server, sizeof(struct sockaddr));
		if (nResult == -1) {
			//Không thể kết nối
			//std::cout << "Khong the ket noi den server lan " << try_count + 1 << std::endl;
			//std::cout << "Error code: " << WSAGetLastError() << std::endl;
			Sleep(500);
			try_count++;
		}
		else {
			error_flag = false;
			try_count = 5;
		}
	}
	if (error_flag)
		return 1;
	return 0;
}