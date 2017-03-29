// Cmd.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "..\..\DualCnn\DualCnnDll.h"
#include "..\..\DbCnn\DbCnnDll.h"
#include "..\..\DbCnn_Ora\OraDbCnnDll.h"
#include "..\CmdDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef struct _tag_CmdInfo
{
	CString m_sCmdName;
	CString m_sMainPart;
	CString m_sWherePart;
	CString m_sOrderPart;
} CmdInfo;

CCriticalSection g_cs;
BOOL g_bIsInitialized = 0;
CMap<CString, LPCTSTR, CmdInfo, CmdInfo&> g_CmdMap;
MSG_INFO g_LastMsg(MSG_INFORMATION, OK, _T(""), CMD_DLL_NAME);

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE2("%s Initializing at 0x%X!\n", CMD_DLL_NAME, hInstance);
		g_hModuleHandle = hInstance;
		SetResourceHandle(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE1("%s Terminated!\n", CMD_DLL_NAME);
	}
	return 1;
}

DWORD __stdcall CmdGetVersion()
{
	return CMD_DLL_VERSION;
}

void _InternalInitialize(CSockRs& rs, LPCTSTR pCmdColumn, LPCTSTR pMainPartColumn, LPCTSTR pWherePartColumn, LPCTSTR pOrderPartColumn)
{
	g_CmdMap.RemoveAll();
	for(rs.MoveFirst(); !rs.IsEof(); rs.MoveNext())
	{
		CmdInfo ci;
		ci.m_sCmdName = rs.GetValue(pCmdColumn).GetString();
		ci.m_sMainPart = rs.GetValue(pMainPartColumn).GetString();
		if (pWherePartColumn && _tcslen(pWherePartColumn) > 0)
		{
			CSockRsField fld = rs.GetValue(pWherePartColumn);
			ci.m_sWherePart = fld.GetType() == ftEmpty ? _T("") : fld.GetString();
		}
		if (pOrderPartColumn && _tcslen(pOrderPartColumn) > 0)
		{
			CSockRsField fld = rs.GetValue(pOrderPartColumn);
			ci.m_sOrderPart = fld.GetType() == ftEmpty ? _T("") : fld.GetString();
		}

		g_CmdMap.SetAt(ci.m_sCmdName, ci);
	}
}

BOOL __stdcall CmdInitialize(Object_Id DualCnnId, LPCTSTR pTableName, LPCTSTR pCmdColumn, LPCTSTR pMainPartColumn, LPCTSTR pWherePartColumn, LPCTSTR pOrderPartColumn, MSG_STRUCT* pMsg)
{
	g_cs.Lock();
	g_LastMsg.set_msg(MSG_INFORMATION, OK, _T(""));
	try
	{
		if (0 >= g_bIsInitialized)
		{
			CString sModulePath;
			if (NULL != pTableName && _tcslen(pTableName) > 0 &&
				NULL != pCmdColumn && _tcslen(pCmdColumn) > 0 &&
				NULL != pMainPartColumn && _tcslen(pMainPartColumn) > 0 &&
				ERROR_SUCCESS == ::GetModulePath(&sModulePath))
			{
				CDualCnn dc(CMD_DLL_NAME);
				dc.DllLoad(sModulePath);
				// Создать копию
				dc.CreateAddRef(DualCnnId);

				CString sFields = CString(pCmdColumn) + _T(", ") + CString(pMainPartColumn);
				if (pWherePartColumn && _tcslen(pWherePartColumn) > 0)
					sFields += _T(", ") + CString(pWherePartColumn);
				if (pOrderPartColumn && _tcslen(pOrderPartColumn) > 0)
					sFields += _T(", ") + CString(pOrderPartColumn);

				CString sCmd = FormatString(_T("select %s from %s"), sFields, pTableName);
				CSockRs rs = dc.ExecuteGet(sCmd);

				_InternalInitialize(rs, pCmdColumn, pMainPartColumn, pWherePartColumn, pOrderPartColumn);

				g_bIsInitialized = 1;
			}
		}
		else
			g_bIsInitialized++;

	}
	catch(MSG_INFO* p_mi)
	{
		std::auto_ptr<MSG_INFO> ap(p_mi);
		g_LastMsg.set_msg(*p_mi);
	}
	catch(...)
	{
		g_LastMsg.set_msg(MSG_ERROR, E_UNHANDLED, S_E_UNHANDLED);
	}

	*pMsg = g_LastMsg;
	g_cs.Unlock();
	
	return (g_bIsInitialized > 0);
}

BOOL __stdcall CmdInitialize2(Object_Id DbCnnId, LPCTSTR pTableName, LPCTSTR pCmdColumn, LPCTSTR pMainPartColumn, LPCTSTR pWherePartColumn, LPCTSTR pOrderPartColumn, MSG_STRUCT* pMsg)
{
	g_cs.Lock();
	g_LastMsg.set_msg(MSG_INFORMATION, OK, _T(""));
	try
	{
		if (0 >= g_bIsInitialized)
		{
			CString sModulePath;
			if (NULL != pTableName && _tcslen(pTableName) > 0 &&
				NULL != pCmdColumn && _tcslen(pCmdColumn) > 0 &&
				NULL != pMainPartColumn && _tcslen(pMainPartColumn) > 0 &&
				ERROR_SUCCESS == ::GetModulePath(&sModulePath))
			{
				CDbCnn dbc(CMD_DLL_NAME);
				dbc.DllLoad(sModulePath);
				// Создать копию
				dbc.CreateAddRef(DbCnnId);

				CString sFields = CString(pCmdColumn) + _T(", ") + CString(pMainPartColumn);
				if (pWherePartColumn && _tcslen(pWherePartColumn) > 0)
					sFields += _T(", ") + CString(pWherePartColumn);
				if (pOrderPartColumn && _tcslen(pOrderPartColumn) > 0)
					sFields += _T(", ") + CString(pOrderPartColumn);

				CString sCmd = FormatString(_T("select %s from %s"), sFields, pTableName);
				CSockRs rs = dbc.ExecuteGet(sCmd);

				_InternalInitialize(rs, pCmdColumn, pMainPartColumn, pWherePartColumn, pOrderPartColumn);

				g_bIsInitialized = 1;
			}
		}
		else
			g_bIsInitialized++;
	}
	catch(MSG_INFO* p_mi)
	{
		std::auto_ptr<MSG_INFO> ap(p_mi);
		g_LastMsg.set_msg(*p_mi);
	}
	catch(...)
	{
		g_LastMsg.set_msg(MSG_ERROR, E_UNHANDLED, S_E_UNHANDLED);
	}

	*pMsg = g_LastMsg;
	g_cs.Unlock();

	return (g_bIsInitialized > 0);
}

BOOL __stdcall CmdInitialize3(Object_Id OraDbCnnId, LPCTSTR pTableName, LPCTSTR pCmdColumn, LPCTSTR pMainPartColumn, LPCTSTR pWherePartColumn, LPCTSTR pOrderPartColumn, MSG_STRUCT* pMsg)
{
	g_cs.Lock();
	g_LastMsg.set_msg(MSG_INFORMATION, OK, _T(""));
	try
	{
		if (0 >= g_bIsInitialized)
		{
			CString sModulePath;
			if (NULL != pTableName && _tcslen(pTableName) > 0 &&
				NULL != pCmdColumn && _tcslen(pCmdColumn) > 0 &&
				NULL != pMainPartColumn && _tcslen(pMainPartColumn) > 0 &&
				ERROR_SUCCESS == ::GetModulePath(&sModulePath))
			{
				COraDbCnn dbc(CMD_DLL_NAME);
				dbc.DllLoad(sModulePath);
				// Создать копию
				dbc.CreateAddRef(OraDbCnnId);

				CString sFields = CString(pCmdColumn) + _T(", ") + CString(pMainPartColumn);
				if (pWherePartColumn && _tcslen(pWherePartColumn) > 0)
					sFields += _T(", ") + CString(pWherePartColumn);
				if (pOrderPartColumn && _tcslen(pOrderPartColumn) > 0)
					sFields += _T(", ") + CString(pOrderPartColumn);

				CString sCmd = FormatString(_T("select %s from %s"), sFields, pTableName);
				CSockRs rs = dbc.ExecuteCmdGet(sCmd);

				_InternalInitialize(rs, pCmdColumn, pMainPartColumn, pWherePartColumn, pOrderPartColumn);

				g_bIsInitialized = 1;
			}
		}
		else
			g_bIsInitialized++;
	}
	catch(MSG_INFO* p_mi)
	{
		std::auto_ptr<MSG_INFO> ap(p_mi);
		g_LastMsg.set_msg(*p_mi);
	}
	catch(...)
	{
		g_LastMsg.set_msg(MSG_ERROR, E_UNHANDLED, S_E_UNHANDLED);
	}

	*pMsg = g_LastMsg;
	g_cs.Unlock();

	return (g_bIsInitialized > 0);
}

void __stdcall CmdUninitialize()
{
	g_cs.Lock();
	g_bIsInitialized--;
	if (g_bIsInitialized <= 0)
		g_CmdMap.RemoveAll();
	g_cs.Unlock();
}

BOOL __stdcall CmdGet(LPCTSTR pCmdName, LPCTSTR* pMainPart, LPCTSTR* pWherePart, LPCTSTR* pOrderPart)
{
	g_cs.Lock();
	BOOL bResult = FALSE;

	if (g_bIsInitialized > 0)
	{
		CmdInfo pci;
		if (g_CmdMap.Lookup(pCmdName, pci))
		{
			if (pMainPart)
				*pMainPart = pci.m_sMainPart;
			if (pWherePart)
				*pWherePart = pci.m_sWherePart;
			if (pOrderPart)
				*pOrderPart = pci.m_sOrderPart;

			bResult = TRUE;
		}
	}

	g_cs.Unlock();
	return bResult;
}

