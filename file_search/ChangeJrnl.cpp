/******************************************************************************
Module name: ChangeJrnl.cpp
Written by: Jeffrey Cooperstein & Jeffrey Richter
******************************************************************************/
#include "stdafx.h"
#include "ChangeJrnl.h"


///////////////////////////////////////////////////////////////////////////////

#define DATETIMESEPERATOR TEXT(", ")

BOOL CChangeJrnl::GetDateTimeFormatFromFileTime(__int64 filetime,
   LPTSTR pszDateTime, int cchDateTime) {

   // This function formats a 64 bit file time into a human readable format
   // expressed as the local date/time.
   // The resolution of the filetime parameter is actually 100 nanoseconds,
   // but this function only formats the output to a resolution of 1 second.

   // Convert file time to system time
   SYSTEMTIME st;
   FileTimeToSystemTime((FILETIME*)&filetime, &st);

   // Convert system time to local time
   SystemTimeToTzSpecificLocalTime(NULL, &st, &st);

   int cch;

   // Get date format
   cch = GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL,
      pszDateTime, cchDateTime);

   // Decrease available buffer size by date length (excluding the NULL), and
   // the length of the date/time seperator
   cchDateTime -= (cch-1) + lstrlen(DATETIMESEPERATOR);

   // Make sure GetDateFormat succeeded, and there's still enough space for
   // the date/time seperator with some space left over for the time string.
   if(cch == 0 || cchDateTime <= 1) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   // Append date/time seperator
   lstrcat(pszDateTime, DATETIMESEPERATOR);

   // Move pointer to end of string
   pszDateTime += lstrlen(pszDateTime);

   // Append the time
   cch = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL,
      pszDateTime, cchDateTime);

   return(cch != 0);
}


///////////////////////////////////////////////////////////////////////////////


BOOL CChangeJrnl::GetReasonString(DWORD dwReason, LPTSTR pszReason,
   int cchReason) {

   // This function converts reason codes into a human readable form
   static LPCTSTR szCJReason[] = {
      TEXT("DataOverwrite"),         // 0x00000001
      TEXT("DataExtend"),            // 0x00000002
      TEXT("DataTruncation"),        // 0x00000004
      TEXT("0x00000008"),            // 0x00000008
      TEXT("NamedDataOverwrite"),    // 0x00000010
      TEXT("NamedDataExtend"),       // 0x00000020
      TEXT("NamedDataTruncation"),   // 0x00000040
      TEXT("0x00000080"),            // 0x00000080
      TEXT("FileCreate"),            // 0x00000100
      TEXT("FileDelete"),            // 0x00000200
      TEXT("PropertyChange"),        // 0x00000400
      TEXT("SecurityChange"),        // 0x00000800
      TEXT("RenameOldName"),         // 0x00001000
      TEXT("RenameNewName"),         // 0x00002000
      TEXT("IndexableChange"),       // 0x00004000
      TEXT("BasicInfoChange"),       // 0x00008000
      TEXT("HardLinkChange"),        // 0x00010000
      TEXT("CompressionChange"),     // 0x00020000
      TEXT("EncryptionChange"),      // 0x00040000
      TEXT("ObjectIdChange"),        // 0x00080000
      TEXT("ReparsePointChange"),    // 0x00100000
      TEXT("StreamChange"),          // 0x00200000
      TEXT("0x00400000"),            // 0x00400000
      TEXT("0x00800000"),            // 0x00800000
      TEXT("0x01000000"),            // 0x01000000
      TEXT("0x02000000"),            // 0x02000000
      TEXT("0x04000000"),            // 0x04000000
      TEXT("0x08000000"),            // 0x08000000
      TEXT("0x10000000"),            // 0x10000000
      TEXT("0x20000000"),            // 0x20000000
      TEXT("0x40000000"),            // 0x40000000
      TEXT("*Close*")                // 0x80000000
   };
   TCHAR sz[1024];
   sz[0] = sz[1] = sz[2] = 0;
   for (int i = 0; dwReason != 0; dwReason >>= 1, i++) {
      if ((dwReason & 1) == 1) {
         lstrcat(sz, TEXT(", "));
         lstrcat(sz, szCJReason[i]);
      }
   }
   BOOL fOk = FALSE;
   if (cchReason > lstrlen(&sz[2])) {
      lstrcpy(pszReason, &sz[2]);
      fOk = TRUE;
   } else {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
   }

   return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


HANDLE CChangeJrnl::Open(TCHAR cDriveLetter, DWORD dwAccess, BOOL fAsyncIO) {

   // This is a helper function that opens a handle to the volume specified
   // by the cDriveLetter parameter.
   TCHAR szVolumePath[_MAX_PATH];
   wsprintf(szVolumePath, TEXT("\\\\.\\%c:"), cDriveLetter);
   HANDLE hCJ = CreateFile(szVolumePath, dwAccess,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
      (fAsyncIO ? FILE_FLAG_OVERLAPPED : 0), NULL);
   return(hCJ);
}


///////////////////////////////////////////////////////////////////////////////


 void CChangeJrnl::ChangeJournalWaiter() {

   // This is a thread function that waits for asynchronous I/O to complete
   // It is only used to wait for additional data to be written to the journal.
   // When the asynchronous I/O has completed, a message is sent to the
   // application after the specified delay time.

   // The pvParam specifies the instance of the CChangeJrnl class that
   // created this thread
 //  CChangeJrnl *pcj = (CChangeJrnl *) pvParam;

   // Loop waiting for asynchronous I/O to complete
 //  while (TRUE) {

      // Wait for asynchronous I/O to complete,
      WaitForSingleObject(m_oCJAsync.hEvent, INFINITE);

      // The m_hwndApp member is set to NULL if the instance of CChangeJrnl
      // is being destroyed. In this case, we must exit the thread.
     

      // Wait the specified delay before notifying the application
      if (m_dwDelay)
         Sleep(m_dwDelay);

      // Post a message that more data is available in the journal
     // PostMessage(pcj->m_hwndApp, WM_CHANGEJOURNALCHANGED, 0, 0);
  // }
  
}


///////////////////////////////////////////////////////////////////////////////


CChangeJrnl::CChangeJrnl() {

   // Initialize member variables
   m_hCJ = INVALID_HANDLE_VALUE;
   m_hCJAsync = INVALID_HANDLE_VALUE;
   ZeroMemory(&m_oCJAsync, sizeof(m_oCJAsync));
   m_pbCJData = NULL;
   m_pUsnRecord = NULL;
   m_hWaiterThread = NULL;
}


///////////////////////////////////////////////////////////////////////////////


CChangeJrnl::~CChangeJrnl() {

   CleanUp();
}


///////////////////////////////////////////////////////////////////////////////


void CChangeJrnl::CleanUp() {

   // Cleanup the memory and handles we were using
   if (m_hCJ != INVALID_HANDLE_VALUE) 
      CloseHandle(m_hCJ);
   if (m_hCJAsync != INVALID_HANDLE_VALUE) 
      CloseHandle(m_hCJAsync);
   if (m_pbCJData != NULL)
      HeapFree(GetProcessHeap(), 0, m_pbCJData);
   if (m_oCJAsync.hEvent != NULL) {

      // Make sure the helper thread knows that we are exiting. We set
      // m_hwndApp to NULL then signal the overlapped event to make sure the
      // helper thread wakes up.
      m_hwndApp = NULL;
      SetEvent(m_oCJAsync.hEvent);
      CloseHandle(m_oCJAsync.hEvent);
   }
   if (m_hWaiterThread != NULL) {
      // Wait for the helper thread to terminate
      WaitForSingleObject(m_hWaiterThread, INFINITE);
      CloseHandle(m_hWaiterThread);
   }
}


///////////////////////////////////////////////////////////////////////////////


BOOL CChangeJrnl::Create(DWORDLONG MaximumSize, DWORDLONG AllocationDelta) {

   // This function creates a journal on the volume. If a journal already
   // exists this function will adjust the MaximumSize and AllocationDelta
   // parameters of the journal
   DWORD cb;
   CREATE_USN_JOURNAL_DATA cujd;
   cujd.MaximumSize = MaximumSize;
   cujd.AllocationDelta = AllocationDelta;
   BOOL fOk = DeviceIoControl(m_hCJ, FSCTL_CREATE_USN_JOURNAL, 
      &cujd, sizeof(cujd), NULL, 0, &cb, NULL);
   return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


BOOL CChangeJrnl::Delete(DWORDLONG UsnJournalID, DWORD DeleteFlags) {

   // If DeleteFlags specifies USN_DELETE_FLAG_DELETE, the specified journal
   // will be deleted. If USN_DELETE_FLAG_NOTIFY is specified, the function
   // waits until the system has finished the delete process.
   // USN_DELETE_FLAG_NOTIFY can be specified alone to wait for the system
   // to finish if another application has started deleting a journal.
   DWORD cb;
   DELETE_USN_JOURNAL_DATA dujd;
   dujd.UsnJournalID = UsnJournalID;
   dujd.DeleteFlags = DeleteFlags;
   BOOL fOk = DeviceIoControl(m_hCJ, FSCTL_DELETE_USN_JOURNAL, 
      &dujd, sizeof(dujd), NULL, 0, &cb, NULL);
   return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


BOOL CChangeJrnl::Query(PUSN_JOURNAL_DATA pUsnJournalData) {

   // Return statistics about the journal on the current volume
   DWORD cb;
   BOOL fOk = DeviceIoControl(m_hCJ, FSCTL_QUERY_USN_JOURNAL, NULL, 0, 
      pUsnJournalData, sizeof(*pUsnJournalData), &cb, NULL);
   return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


BOOL CChangeJrnl::Init(TCHAR cDriveLetter, DWORD cbBuffer) {

   // Call this to initialize the structure. The cDriveLetter parameter
   // specifies the drive that this instance will access. The cbBuffer
   // parameter specifies the size of the interal buffer used to read records
   // from the journal. This should be large enough to hold several records
   // (for example, 10 kilobytes will allow this class to buffer several
   // dozen journal records at a time)

   // You should not call this function twice for one instance.
   if (m_pbCJData != NULL) DebugBreak();

   m_cDriveLetter = cDriveLetter;
   BOOL fOk = FALSE;
   __try {
      // Allocate internal buffer
      m_pbCJData = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbBuffer);
      if (NULL == m_pbCJData) __leave;

      // Open a handle to the volume
      m_hCJ = Open(cDriveLetter, GENERIC_WRITE | GENERIC_READ, FALSE);
      if (INVALID_HANDLE_VALUE == m_hCJ) __leave;

      // Open a handle to the volume for asynchronous I/O. This is used to wait
      // for new records after all current records have been processed
      m_hCJAsync = Open(cDriveLetter, GENERIC_WRITE | GENERIC_READ, TRUE);
      if (INVALID_HANDLE_VALUE == m_hCJAsync) __leave;

      // Create an event for asynchronous I/O.
      m_oCJAsync.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      if (NULL == m_oCJAsync.hEvent) __leave;

       // Create a thread that waits for asynchronous I/O to complete
	   //  m_hWaiterThread = CreateThread(NULL, 0, ChangeJournalWaiter, this,
       //    0, NULL);
       //if (NULL == m_hWaiterThread) __leave;

      fOk = TRUE;
   }
   __finally {
      if (!fOk) CleanUp();
   }
   return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


void CChangeJrnl::SeekToUsn(USN usn, DWORD ReasonMask,
   DWORD ReturnOnlyOnClose, DWORDLONG UsnJournalID) {

   // This function starts the process of reading data from the journal. The
   // parameters specify the initial location and filters to use when
   // reading the journal. The usn parameter may be zero to start reading
   // at the start of available data. The EnumNext function is called to
   // actualy get journal records

   // Store the parameters in m_rujd. This will determine
   // how we load buffers with the EnumNext function.
   m_rujd.StartUsn          = usn;
   m_rujd.ReasonMask        = ReasonMask;
   m_rujd.ReturnOnlyOnClose = ReturnOnlyOnClose;
   m_rujd.Timeout           = 0;
   m_rujd.BytesToWaitFor    = 0;
   m_rujd.UsnJournalID      = UsnJournalID;
   m_cbCJData = 0;
   m_pUsnRecord = NULL;
}


///////////////////////////////////////////////////////////////////////////////


PUSN_RECORD CChangeJrnl::EnumNext() {

   // Make sure we have a buffer to use
   if (m_pbCJData == NULL) DebugBreak();

   // This will return the next record in the journal that meets the
   // requirements specified with the SeekToUsn function. If no more
   // records are available, the functions returns NULL immediately
   // (since BytesToWaitFor is zero). This function will also return
   // NULL if there is an error loading a buffer with the DeviceIoControl
   // function. Use the GetLastError function to determine the cause of the
   // error.
   // If the EnumNext function returns NULL, GetLastError may return one
   // of the following.
   //   S_OK - There was no error, but there are no more available journal
   //          records. Use the NotifyMoreData function to wait for more
   //          data to become available.
   //   ERROR_JOURNAL_DELETE_IN_PROGRESS - The journal is being deleted. Use
   //          the Delete function with USN_DELETE_FLAG_NOTIFY to wait for
   //          this process to finish. Then, call the Create function and then
   //          SeekToUsn to start reading from the new journal.
   //   ERROR_JOURNAL_NOT_ACTIVE - The journal has been deleted. Call the
   //          Create function then SeekToUsn to start reading from the new
   //          journal
   //   ERROR_INVALID_PARAMETER - Possibly caused if the journal's ID has
   //          changed. Flush all cached data. Call the Query function and then
   //          SeekToUsn to start reading from the new journal
   //   ERROR_JOURNAL_ENTRY_DELETED - Journal records were purged from the
   //          journal before we got a chance to process them. Cached
   //          information should be flushed.

   // If we do not have a record loaded, or enumerating to the next record
   // will point us past the end of the output buffer returned
   // by DeviceIoControl, we need to load a new block of data into memory
   if (   (NULL == m_pUsnRecord)
       || ((PBYTE) m_pUsnRecord + m_pUsnRecord->RecordLength)
             >= (m_pbCJData + m_cbCJData)) {

      m_pUsnRecord = NULL;

      BOOL fOk = DeviceIoControl(m_hCJ, FSCTL_READ_USN_JOURNAL,
         &m_rujd, sizeof(m_rujd), m_pbCJData,
         HeapSize(GetProcessHeap(), 0, m_pbCJData), &m_cbCJData, NULL);
	  int i = HeapSize(GetProcessHeap(), 0, m_pbCJData);
	  HRESULT error_code = GetLastError();
      if (fOk) {
         // It is possible that DeviceIoControl succeeds, but has not
         // returned any records - this happens when we reach the end of
         // available journal records. We return NULL to the user if there's
         // a real error, or if no records are returned.

         // Set the last error to NO_ERROR so the caller can distinguish
         // between an error, and the case where no records were returned.
         SetLastError(NO_ERROR);

         // Store the 'next usn' into m_rujd.StartUsn for use the
         // next time we want to read from the journal
         m_rujd.StartUsn = * (USN *) m_pbCJData;

         // If we got more than sizeof(USN) bytes, we must have a record.
         // Point the current record to the first record in the buffer
         if (m_cbCJData > sizeof(USN))
            m_pUsnRecord = (PUSN_RECORD) &m_pbCJData[sizeof(USN)];
      }
   } else {
      // The next record is already available in our stored
      // buffer - Move pointer to next record
      m_pUsnRecord = (PUSN_RECORD) 
         ((PBYTE) m_pUsnRecord + m_pUsnRecord->RecordLength);
   }
   return(m_pUsnRecord);
}


///////////////////////////////////////////////////////////////////////////////


bool CChangeJrnl::NotifyMoreData( DWORD dwDelay) {

   // This function should be called if the EnumNext function returned NULL,
   // but GetLastError returns NO_ERROR (the end of available records was 
   // reached). This function will send a WM_CHANGEJOURNALCHANGED message when
   // more data is available. The application can then start calling the 
   // EnumNext function to retrieve the new data. The dwDelay parameter 
   // specifies the number of milliseconds to wait before sending the 
   // WM_CHANGEJOURNALCHANGED message. This is useful to prevent the
   // application from competing for resources as it tries to read the
   // journal if another application is causing a number of journal entries
   // to be created

 //  m_hwndApp = hwndApp;
   m_dwDelay = dwDelay;

   READ_USN_JOURNAL_DATA_V0 rujd;
   rujd = m_rujd;
   rujd.BytesToWaitFor = 1;

   // Try to read at least one byte from the journal at the specified
   // USN. When 1 byte is available, the event in m_oCJAsync will
   // be signaled
   BOOL fOk = DeviceIoControl(m_hCJAsync, FSCTL_READ_USN_JOURNAL, &rujd, 
      sizeof(rujd), &m_UsnAsync, sizeof(m_UsnAsync), NULL, &m_oCJAsync);
   if(!fOk && (GetLastError() != ERROR_IO_PENDING)) {
      // Notify the app immediately since we did not successfully
      // set up the asynchronous read.
      //PostMessage(m_hwndApp, WM_CHANGEJOURNALCHANGED, 0, 0);
	   return false;
   }
   WaitForSingleObject(m_oCJAsync.hEvent, INFINITE);

   // The m_hwndApp member is set to NULL if the instance of CChangeJrnl
   // is being destroyed. In this case, we must exit the thread.


   // Wait the specified delay before notifying the application
	Sleep(m_dwDelay);
	return true;

}


///////////////////////////////// End Of File /////////////////////////////////

