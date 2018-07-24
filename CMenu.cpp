#include "stdafx.h"

using namespace std;

CMenu::CMenu(Program *prog) {
	option.push_back("Quit");
	function.push_back(nullptr);
	p = prog;
}


void CMenu::Add(string name, int(Program::*f)()) {
	option.insert(option.end() - 1, name);
	function.insert(function.end() - 1, f);
}

void CMenu::Show() {
	int i = 0;
	int n = option.size();

	for (; i < n; i++) {
		cout << i + 1 << ". ";
		cout << option[i] << endl;
	}
	cout << endl;
}

int CMenu::Select() {
	string s;
	int i;
	cout << "Ban lua chon: (Ghi Bang So)  ";
	cin >> s;
	if (s == "m" || s == "menu" || s == "help")
		return 400;

	i = stoi(s);
	if (i == option.size())
		return 404;

	//Đừng đọc :v
	//f là hàm "giả" của class Program. Gán hàm này bằng 1 hàm thật rồi chạy nó
	p->f = function[i - 1];
	return (p->*(p->f))();
}