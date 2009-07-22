/*
This file is part of GPUP, which is part of Plugin Manager 
Plugin for Notepad++

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

#include "stdafx.h"
#include "gpup.h"

#include <tchar.h>
#include <string>
#include <list>
#include <boost/shared_ptr.hpp>

#pragma warning (push)
#pragma warning (disable : 4512) // assignment operator could not be generated
#include <boost/bind.hpp>
#pragma warning (pop)

#include <boost/function.hpp>


#include "Options.h"
#include "tinyxml/tinyxml.h"
#include "libinstall/InstallStep.h"
#include "libinstall/InstallStepFactory.h"
#include "ProgressDialog.h"

#define RETURN_SUCCESS				0
#define RETURN_INVALID_PARAMETERS   1
#define RETURN_CANCELLED			2
#define RETURN_ACTIONERROR			3


#define MAX_WRITE_ATTEMPTS			10

// Global Variables:
HINSTANCE	   hInst;								// current instance
ProgressDialog *g_progressDialog;


using namespace std;
using namespace boost;


BOOL parseCommandLine(const TCHAR* cmdLine, Options& options)
{
	/* Command line options are
	 * -a <actionsFile.xml>
	 * -w <windowName>
	 * -e <exeToRun>
	 *
     * w and e are mandatory 
	 */

	if (!cmdLine)
		return TRUE;

	
	
	
	tstring* arg;
	arg = new tstring;

	list<tstring*> argList;

	enum PARSEMODE
	{
		PARSEMODE_START,
		PARSEMODE_STANDARD,
		PARSEMODE_QUOTED
	} parseMode;

	parseMode = PARSEMODE_START;

	while(*cmdLine)
	{

		switch(parseMode)
		{
			case PARSEMODE_START:
				if (*cmdLine == _T('\"'))
				{
					parseMode = PARSEMODE_QUOTED;
				}
				else if (*cmdLine != _T(' '))
				{
					arg->push_back(*cmdLine);
					parseMode = PARSEMODE_STANDARD;
				}
				
				break;


			case PARSEMODE_STANDARD:			
				if (*cmdLine == _T(' '))
				{
					parseMode = PARSEMODE_START;
					argList.push_back(arg);
					arg = new tstring;
				}
				else
				{
					arg->push_back(*cmdLine);
				}
				break;

			case PARSEMODE_QUOTED:
				if (*cmdLine == _T('\"'))
				{
					parseMode = PARSEMODE_START;
					argList.push_back(arg);
					arg = new tstring;
				}
				else
				{
					arg->push_back(*cmdLine);
				}
				break;

		}

		++cmdLine;
	}

	if (!arg->empty())
		argList.push_back(arg);
	
	
	list<tstring*>::iterator iter = argList.begin();
	while(iter != argList.end())
	{
		if (*(*iter) == _T("-w"))
		{
			++iter;
			if (iter != argList.end())
				options.setWindowName((*iter)->c_str());
		}
		else if (*(*iter) == _T("-e"))
		{
			++iter;
			if (iter != argList.end())
				options.setExeName((*iter)->c_str());
		}
		else if (*(*iter) == _T("-a"))
		{
			++iter;
			if (iter != argList.end())
				options.setActionsFile((*iter)->c_str());
		}


		++iter;
	}



	return TRUE;

}


BOOL closeMainProgram(Options &options)
{
	HWND h = ::FindWindowEx(NULL, NULL, options.getWindowName().c_str(), NULL);
	while (h)
	{
		if (!::SendMessage(h, WM_CLOSE, 0, 0))
			return FALSE;

		h = ::FindWindowEx(NULL, NULL, options.getWindowName().c_str(), NULL);
	}

	return TRUE;
}

void startNewInstance(const tstring& exeName)
{

	STARTUPINFO startup;
	memset(&startup, 0, sizeof(STARTUPINFO));

	startup.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION procinfo;
	
    ::CreateProcess((TCHAR *)exeName.c_str(),        
						NULL,         // arguments
						NULL,        // process security
						NULL,        // thread security
						FALSE,        // inherit handles flag
						NULL,           // flags
						NULL,        // inherit environment
						NULL,        // inherit directory
						&startup,    // STARTUPINFO
						&procinfo);  // PROCESS_INFORMATION

}




void showProgressDialog(int stepCount)
{
	g_progressDialog = new ProgressDialog(hInst);
	g_progressDialog->doDialog(stepCount);

	
	
}

void setStatus(const TCHAR* status)
{
	g_progressDialog->setStatus(status);
}

void stepProgress(int /*percentageComplete*/)
{
	
}


BOOL processActionsFile(const tstring& actionsFile)
{


	TiXmlDocument xmlDocument(actionsFile.c_str());
	if (xmlDocument.LoadFile())
	{
		TiXmlElement *install = xmlDocument.FirstChildElement(_T("install"));
		TiXmlElement stillToComplete(_T("install"));
		tstring basePath;

		if (install && !install->NoChildren())
		{
			
			InstallStepFactory installStepFactory(NULL);
			TiXmlElement *step = install->FirstChildElement();
			int stepCount = 0;
			while (step)
			{
				stepCount++;
				step = static_cast<TiXmlElement*>(install->IterateChildren(step));
			} 

			showProgressDialog(stepCount);


			step = install->FirstChildElement();
			while (step)
			{
				shared_ptr<InstallStep> installStep = installStepFactory.create(step, "", 0);


				StepStatus stepStatus;
				stepStatus = installStep->perform(basePath,           // basePath
												&stillToComplete,   // forGpup (still can't achieve, so basically a fail)
												boost::bind(&setStatus, _1),     // status update function
												boost::bind(&stepProgress, _1),
												NULL); // step progress function


				// If it said it needed to do it in GPUP, then maybe N++ hasn't quite
				// finished quitting yet.

				int retryCount = 0;
				while (stepStatus == STEPSTATUS_NEEDGPUP && retryCount < 20)
				{
					::Sleep(500);
					++retryCount;
					stepStatus = installStep->perform(basePath,           // basePath
												&stillToComplete,   // forGpup (still can't achieve, so basically a fail)
												boost::bind(&setStatus, _1),     // status update function
												boost::bind(&stepProgress, _1), // step progress function
												NULL); 



				}

				g_progressDialog->stepComplete();				

				step = (TiXmlElement*) install->IterateChildren(step);
			}

		}	

		::DeleteFile(actionsFile.c_str());

		return TRUE;
	}
	else
		return FALSE;

	

}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(nCmdShow);


	hInst = hInstance;

	Options options;
	
	parseCommandLine(lpCmdLine, options);

	if (options.getWindowName() == _T("")
		|| options.getExeName() == _T(""))
	{
		::MessageBox(NULL, _T("Invalid parameters passed to the plugin updater (GPUP).\r\n")
						   _T("Usage:")
						   _T(" gpup.exe -w <window name> -e <exe name> [-a <actions file>]"), _T("Plugin Update"), MB_OK | MB_ICONERROR);
		return RETURN_INVALID_PARAMETERS;
	}

	BOOL allClosed = closeMainProgram(options);

	if (!allClosed)
	{
		return RETURN_CANCELLED;
	}

	//::MessageBox(NULL, _T("Pause..."), _T("Timing test"), 0);

	if (options.getActionsFile() != _T(""))
	{
		if (!processActionsFile(options.getActionsFile()))
		{
			MessageBox(NULL, _T("Error finishing installation steps.  Plugin installation has not completed successfully."), _T("Plugin Manager"), MB_OK | MB_ICONERROR);
		}
	}

	startNewInstance(options.getExeName());
	

	return RETURN_SUCCESS;

}




