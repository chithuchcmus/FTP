#pragma once
#include "stdafx.h"

int SetupIPWS(SOCKET&, sockaddr_in&, bool = true);
int Connect(SOCKET&, sockaddr_in&);