#pragma once
#include "stdafx.h"


class CMenu {
private:
	//Mảng tên dòng tùy chọn
	std::vector<std::string> option;
	//Mảng các hàm
	std::vector<int(Program::*)()> function;
	//SOCKET hiện hành
	Program *p;
public:
	//Khởi tạo
	CMenu(Program*);
	//Thêm dòng mới
	void Add(std::string name, int(Program::*f)());
	//Hiển thị các dòng tùy chọn
	void Show();
	//Yêu cầu chọn, trả về kết quả của hàm được chọn
	int Select();
};