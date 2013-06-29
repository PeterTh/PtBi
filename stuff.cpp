
#include "stdafx.h"

#include <strsafe.h>
#include <time.h>
#include <sys/stat.h>

const char* timeString() {
	static char buffer[32];
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 32, "%Y-%m-%d_%H-%M-%S", timeinfo);

	return buffer;
}

unsigned int fileSize(const char *name) {
	struct stat fileStats;
	RT_ASSERT(stat(name, &fileStats) == 0, "Could not 'stat' shader file \"" << name << "\" to determine its size.");
	return fileStats.st_size;
}
void readFile(const char *name, char* buff) {
	FILE *file = fopen(name, "r");
	RT_ASSERT(file != NULL,	"Could not open file \"" << name << "\".");
	size_t c = fread(buff, 1, fileSize(name), file);
	buff[c] = '\0';
	fclose(file);
}

void ErrorExit(LPTSTR lpszFunction) { 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}

#include <stdarg.h>

string format(const char* formatString, ...) {
	va_list arglist;
	va_start(arglist, formatString);
	const unsigned BUFFER_SIZE = 2048*8;
	char buffer[BUFFER_SIZE];
	vsnprintf(buffer, BUFFER_SIZE, formatString, arglist);
	va_end(arglist);
	return string(buffer);
}
