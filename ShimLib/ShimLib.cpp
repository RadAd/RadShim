#include "ShimLib.h"
#include "Error.h"
#include <tchar.h>

DWORD Launch(const HINSTANCE hInstance, const LPCTSTR lpCmdLine)
try
{
    HANDLE hJob = NULL;
    CHECK_LE(hJob = CreateJobObject(nullptr, nullptr));
    CHECK_LE(AssignProcessToJobObject(hJob, GetCurrentProcess()));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli = { 0 };
#if 0
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
#else
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
#endif
    CHECK_LE(SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli)));

    TCHAR command[MAX_PATH] = _T("");
    CHECK_LE(LoadString(hInstance, IDS_TARGET, command, ARRAYSIZE(command)));

    TCHAR fullcmd[MAX_PATH * 5] = _T("");
#if 0
    _tcscpy_s(fullcmd, command);
    _tcscat_s(fullcmd, _T(" "));
    _tcscat_s(fullcmd, lpCmdLine);
#else
    _stprintf_s(fullcmd, _T("\"%s\" %s"), command, lpCmdLine);
#endif

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    CHECK_LE(CreateProcess(nullptr, fullcmd, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi));
#if 0
    CHECK_LE(AssignProcessToJobObject(hJob, pi.hProcess));
#else
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    CHECK_LE(SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli)));
#endif

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 0;
    ::GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hJob);

    return exit_code;
}
catch (const Win32Error& e)
{
    ReportError(e);
    return e.code;
}
