#pragma once
#include "ChatWindow.h"

class cApp : public wxApp
{
public:
	cApp();
	~cApp();
public:
	ChatWindow* m_ChatWindow = nullptr;

public:
	virtual bool OnInit();
};

