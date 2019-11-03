///////////////////////////////////////////////////////////////////////////////
// genericdll.cpp : Defines the initialization routines for the DLL
//
// This file is part is part of CANAL (CAN Abstraction Layer)
// http://www.vscp.org)
//
// Copyright (C) 2000-2014
// Ake Hedman, Grodans Paradis AB, <akhe@grodansparadis.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $RCSfile: genericdll.cpp,v $                                       
// $Date: 2005/01/05 12:16:14 $                                  
// $Author: akhe $                                              
// $Revision: 1.3 $ 
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "genericdll.h"
#include "../../../common/canal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

 
/////////////////////////////////////////////////////////////////////////////
// CGenericdllApp

BEGIN_MESSAGE_MAP(CGenericdllApp, CWinApp)
	//{{AFX_MSG_MAP(CGenericdllApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGenericdllApp construction

CGenericdllApp::CGenericdllApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGenericdllApp object

CGenericdllApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CGenericdllApp initialization

BOOL CGenericdllApp::InitInstance()
{
	if ( !AfxSocketInit() ) {
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                             C A N A L -  A P I
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// CanalOpen
//


extern "C" long WINAPI EXPORT CanalOpen( char *pDevice, unsigned long flags )
{
	long h = NULL;

	return h;
}


///////////////////////////////////////////////////////////////////////////////
//  CanalClose
// 

extern "C" int WINAPI EXPORT CanalClose( long handle )
{
	int rv = 0;

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
//  CanalGetLevel
// 

extern "C" unsigned long WINAPI EXPORT CanalGetLevel( long handle )
{
	return ( CANAL_LEVEL_STANDARD );
}


///////////////////////////////////////////////////////////////////////////////
// CanalSend
//

extern "C" int WINAPI EXPORT CanalSend( long handle, PCANALMSG pCanalMsg  )
{
	int rv = 0;

	return rv;
}


///////////////////////////////////////////////////////////////////////////////
// CanalReceive
//

extern "C" int WINAPI EXPORT CanalReceive( long handle,  PCANALMSG pCanalMsg  )
{
	int rv = 0;

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
// CanalDataAvailable
//

extern "C" int WINAPI EXPORT CanalDataAvailable(  long handle  )
{
	int rv = 0;

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
// CanalGetStatus
//

extern "C" int WINAPI EXPORT CanalGetStatus( long handle,  PCANALSTATUS pCanalStatus  )
{
	int rv = 0;

	return rv;
}


///////////////////////////////////////////////////////////////////////////////
// CanalGetStatistics
//

extern "C" int WINAPI EXPORT CanalGetStatistics( long handle,  PCANALSTATISTICS pCanalStatistics  )
{
	int rv = 0;

	return rv;
}


///////////////////////////////////////////////////////////////////////////////
// CanalSetFilter
//

extern "C" int WINAPI EXPORT CanalSetFilter(  long handle, unsigned long filter )
{
	int rv = 0;

	return rv;
}


///////////////////////////////////////////////////////////////////////////////
// CanalSetMask
//

extern "C" int WINAPI EXPORT CanalSetMask(  long handle, unsigned long mask )
{
	int rv = 0;

	return rv;
}


///////////////////////////////////////////////////////////////////////////////
// CanalSetBaudrate
//

extern "C" int WINAPI EXPORT CanalSetBaudrate(  long handle, unsigned long baudrate )
{
	int rv = 0;

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
// CanalGetVersion
//

unsigned long WINAPI EXPORT CanalGetVersion( void )
{
	unsigned long version;
	unsigned char *p = (unsigned char *)&version;

	*p = CANAL_MAIN_VERSION;
	*(p+1) = CANAL_MINOR_VERSION;
	*(p+2) = CANAL_SUB_VERSION;
	*(p+3) = 0;
	return version;
}


///////////////////////////////////////////////////////////////////////////////
// CanalGetDllVersion
//

unsigned long WINAPI EXPORT CanalGetDllVersion( void )
{
	return DLL_VERSION;
}


///////////////////////////////////////////////////////////////////////////////
// CanalGetVendorString
//

const char * WINAPI EXPORT CanalGetVendorString( void )
{
	return CANAL_DLL_VENDOR;
}

