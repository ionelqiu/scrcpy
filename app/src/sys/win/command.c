#include "command.h"

#include "config.h"
#include "log.h"
#include "str_util.h"

enum process_result cmd_execute(const char *path, const char *const argv[], HANDLE *handle) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    // Windows command-line parsing is WTF:
    // <http://daviddeley.com/autohotkey/parameters/parameters.htm#WINPASS>
    // only make it work for this very specific program
    // (don't handle escaping nor quotes)
    char cmd[256];
    size_t ret = xstrjoin(cmd, argv, ' ', sizeof(cmd));
    if (ret >= sizeof(cmd)) {
        LOGE("Command too long (%" PRIsizet " chars)", sizeof(cmd) - 1);
        *handle = NULL;
        return PROCESS_ERROR_GENERIC;
    }

#ifdef WINDOWS_NOCONSOLE
    int flags = CREATE_NO_WINDOW;
#else
    int flags = 0;
#endif
    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi)) {
        *handle = NULL;
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return PROCESS_ERROR_MISSING_BINARY;
        }
        return PROCESS_ERROR_GENERIC;
    }

    *handle = pi.hProcess;
    return PROCESS_SUCCESS;
}

SDL_bool cmd_terminate(HANDLE handle) {
    return TerminateProcess(handle, 1) && CloseHandle(handle);
}

SDL_bool cmd_simple_wait(HANDLE handle, DWORD *exit_code) {
    DWORD code;
    if (WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0 || !GetExitCodeProcess(handle, &code)) {
        // cannot wait or retrieve the exit code
        code = -1; // max value, it's unsigned
    }
    if (exit_code) {
        *exit_code = code;
    }
    return !code;
}
