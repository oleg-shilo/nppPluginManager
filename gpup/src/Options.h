/*
This file is part of Plugin Manager Plugin for Notepad++
Copyright (C)2009 Dave Brotherstone <davegb@pobox.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "tstring.h"

class Options
{
public:
	Options()  {};
	~Options() {};

	void setActionsFile(const TCHAR* actionsFile);
	void setWindowName(const TCHAR* windowName);
	void setExeName(const TCHAR* exeName);

	const tstring& getActionsFile();
	const tstring& getExeName();
	const tstring& getWindowName();

private: 
	tstring _actionsFile;
	tstring _windowName;
	tstring _exeName;

	
};

#endif