/******************************************************************************
Module name: ChangeJrnl.h
Written by: Jeffrey Cooperstein & Jeffrey Richter
******************************************************************************/

#pragma once

///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>


///////////////////////////////////////////////////////////////////////////////


#define WM_CHANGEJOURNALCHANGED (WM_USER + 0x100)


///////////////////////////////////////////////////////////////////////////////


class CChangeJrnl {
public:
   CChangeJrnl();
   ~CChangeJrnl();

   BOOL Init(TCHAR cDriveLetter, DWORD cbBuffer);
   BOOL Create(DWORDLONG MaximumSize, DWORDLONG AllocationDelta);
   BOOL Delete(DWORDLONG UsnJournalID, DWORD DeleteFlags);
   BOOL Query(PUSN_JOURNAL_DATA pUsnJournalData);

   void SeekToUsn(USN usn, DWORD ReasonMask,
      DWORD ReturnOnlyOnClose, DWORDLONG UsnJournalID);

   PUSN_RECORD EnumNext();
   bool NotifyMoreData( DWORD dwDelay);

   operator HANDLE() const { return(m_hCJ); }
   TCHAR GetDriveLetter() { return(m_cDriveLetter); }
   DWORDLONG GetCurUsnJrnlID() { return(m_rujd.UsnJournalID); }
   USN GetCurUsn() { return(m_rujd.StartUsn); }
   void  ChangeJournalWaiter();

public:  // Public static helper functions
   static BOOL GetDateTimeFormatFromFileTime(__int64 filetime,
      LPTSTR pszDateTime, int cchDateTime);
   static BOOL GetReasonString(DWORD dwReason, LPTSTR pszReason,
      int cchReason);

private:
   void CleanUp();

public:
   TCHAR  m_cDriveLetter; // drive letter of volume
   HANDLE m_hCJ;          // handle to volume

   // Members used to enumerate journal records
   READ_USN_JOURNAL_DATA_V0 m_rujd;       // parameters for reading records
   PBYTE                 m_pbCJData;   // buffer of records
   DWORD                 m_cbCJData;   // valid bytes in buffer
   PUSN_RECORD           m_pUsnRecord; // pointer to current record

   // Members used to notify application of new data
   HANDLE     m_hCJAsync;      // Async handle to volume
   OVERLAPPED m_oCJAsync;      // overlapped structure
   USN        m_UsnAsync;      // output buffer for overlapped I/O
   HWND       m_hwndApp;       // window to notify of new data
   DWORD      m_dwDelay;       // delay before sending notification
   HANDLE     m_hWaiterThread; // thread that waits for new data

private: // Private static helper functions
   static HANDLE Open(TCHAR cDriveLetter, DWORD dwAccess, BOOL fAsyncIO);
};


