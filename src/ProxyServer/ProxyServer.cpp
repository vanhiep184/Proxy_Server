// ProxyServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include "ProxyServer.h"
#include<WinSock2.h>
#include<stdlib.h>
#include<stdio.h>
#include<string>
#include <stack>
#include <fstream>
#include "Cache.h"
#include "CacheItem.h"
#include"afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----Tham khảo:
//https://github.com/annguyen011197/Proxy-Server?fbclid=IwAR3jmwmhD60yV8MTEtibpHyMrapZr3veb505j41PREnmfKCaqVV1sYfV5qw

#define HTTP  "http://"		//Protocal HTTP
#define HTTP1 "HTTP/1.1"	//phiên bản 1.1
#define PORT	8888    	//Proxy Listen Port
#define BUFSIZE	10240      //Buffer size 2^17
#define PORTSERVER 80


using namespace std;

struct PROXY
{
	SOCKET withClient;	// Dùng để kết nối với browser
	SOCKET withServer;	// Dùng để kết nối với web server
	string URL;			// The first URL  wanna connect
};

//Dùng cho phiên bản 1.0
UINT ConnectVersion1_0(PROXY *PX, char Buf[], int Rec);
UINT AcceptBrowser(LPVOID);
SOCKET TryConnectServer(string s);
UINT ConnectServer(LPVOID);
UINT BreakPROXY(LPVOID );

//Tạo đối tượng singleton
Cache* Cache::m_instance = NULL;
Cache* cache = Cache::getInstance();
//Socket lưu trữ socket được khởi tạo để lắng nghe
SOCKET MainProxy;
//Danh sách cái web bị ban
string webBanned;
//Message cho client biết web bị ban
//string Banned = "<p><strong><span style=\"color: #ff0000; \">EROR:</span></strong></p><h1><em> 404 Forbidden<p>&nbsp; ";
string Banned = "HTTP/1.1 403 Forbidden by Proxy\r\nConnection : close\r\n\r\n <!DOCTYPE HTML PUBLIC \" -//IETF//DTD HTML 2.0//EN\">\r\n<html><head>\r\n<title>403 Forbidden</title>\r\n</head><body>\r\n<h1>Forbidden</h1>\r\n<p>The requested URL /t.html was Forbidden by Proxy Server.</p>\r\n</body></html>\0";

//Khởi tạo Socket ở port 8888
void StartUpProxy()
{

	//Socket với port 8888 để lắng nghe các kết nối
	SOCKET ProxyServer;

	//Địa chỉ cục bộ cho Socket
	sockaddr_in addProxy;
	addProxy.sin_port = htons(PORT);
	addProxy.sin_family = AF_INET;
	addProxy.sin_addr.s_addr = inet_addr("127.0.0.1");

	//Tạo socket ->> SOCK_STREAM(TCP)
	ProxyServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Nếu khởi tạo thất bại
	if (ProxyServer == INVALID_SOCKET)
	{
		wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
		return;
	}

	//Nếu khởi tạo thành công thì bind
	//Kiểm tra có gọi hàm bind thành công không
	if (bind(ProxyServer, (sockaddr*)&addProxy, sizeof(addProxy)) != 0)
	{
		wprintf(L"bind failed with error %u\n", WSAGetLastError());
		closesocket(ProxyServer);
		return;
	}

	//bắt đầu hàm lắng nghe kết nối (tối đa 255)
	if (listen(ProxyServer, 255) != 0)
	{
		//Nếu không thành công
		wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
		closesocket(ProxyServer);
		return;
	}

	MainProxy = ProxyServer;
	//Biến để cho proxy chạy nếu được ngắt kết nối giữa chừng
	bool live = 1;
	//Chạy 1 tuyến trình nhận kí tự kết thúc là e
	AfxBeginThread(BreakPROXY, (LPVOID)&live);
	//Chấp nhận kết nối
	while (live)
		AcceptBrowser(NULL);
	
}

//Hàm nhận kí tự kết thúc là e
UINT BreakPROXY(LPVOID pParam)
{
	bool *live = (bool*)pParam;
	char c;
	c = getchar();
	if (c == 'e') 
	{
		*live = 0;
		closesocket(MainProxy);
		WSACleanup();
		exit(1);
	}
	return 0;
}


//lấy tên từ file vừa nhận được
int GetNameServer(const char Buf[], int len,string &s,string &URL)
{
	//
	char command[50], Domain[100000], proto[20], *p;
	int j;
	sscanf(Buf, "%s%s%s", command, Domain, proto);

	//Check xem có phải là file http
	p = strstr(Domain, HTTP);

	if (p)
	{
		URL.clear();
		//Lấy URL
		URL += Domain;
		
		//Tách tên ra
		p += 7;
		int i = 7;
		for (; i < strlen(Domain); i++)
		{
			if (Domain[i] == '/')
			{
				break;
			}
		}
		Domain[i] = 0;
		s = s + p;
		//Kiểm tra có trong blacklist
		char * checkBan = NULL;
		checkBan = (char*)strstr(webBanned.c_str(), s.c_str());
		//Nếu có sẽ không cho kết nối
		if (checkBan)
		{
			return -2;
		}

		p = NULL;
		p = strstr(proto, HTTP1);
		if (p==NULL)
		{
		// Gửi theo version HTTP 1.0
		//	p = strstr((char*)Buf, HTTP1);
		//	p += (strlen(HTTP1)-1);
		//	*p = '0';
			return 10;
		}
		return 11;
	}
	return -1;

}

UINT AcceptBrowser(LPVOID pParam)
{
	
	//Khởi tạo socket kết nối
	SOCKET SocketAccept;
	CWinThread* pChildThread;
	//Địa chỉ cho các socket kết nối
	sockaddr_in addSocket;
	int sizeAdd = sizeof(addSocket);

	//Xuất thông báo chờ accpet
//	wprintf(L"Waiting for client to connect...\n");

	// accept the connection
	SocketAccept = accept(MainProxy, (sockaddr*)&addSocket, &sizeAdd);
	AfxBeginThread(AcceptBrowser, NULL);
//	::WaitForSingleObject(pChildThread->m_hThread, 20000);
	
	if (SocketAccept == INVALID_SOCKET)
	{
		wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
		closesocket(SocketAccept);
		return 1;
	}
	PROXY PX;
	PX.withClient = SocketAccept;
	//Dùng để lưu tập tin lấy được
	char Buf[BUFSIZE];
	int Rec;
	//Socket cho client
	
	
	//Lấy gói dữ liệu được gửi lên socket (HTTP request CONNECT)
	Rec = recv(SocketAccept, Buf, BUFSIZE, 0);

	string s = "";
	string URL = "";
	//Kết quả trong lấy gói dữ liệu
	int result;
	if (Rec != SOCKET_ERROR)
	{

		//	Tách lấy gói GET để lấy tên (Vì giao thức HTTP luôn bắt đầu bằng http://) 
		result = GetNameServer(Buf, Rec, s,URL);
		// Không là tập tin HTTP thì không cho kết nối
		if (result == -1)
		{
			closesocket(SocketAccept);
			return 1;
		}
		// Hoặc là tập tin HTTP nhưng nằm trong blacklist thì gửi mess xuất ra mành thông báo
		else if (result == -2)
		{
			send(SocketAccept, Banned.c_str(), Banned.size(), 0);
			closesocket(SocketAccept);
			return 1;
		}
	}
	else
	{
		wprintf(L"Error recv file");
		closesocket(SocketAccept);
		return 1;
	}
	
	//Kiểm tra xem URL có đang tồn tại trong cache
	CacheItem* item = cache->access(URL);
	if (item == nullptr); 
	//Gửi dữ liệu xuống cho client khi URL đang tồn tại trong cache
	else {
		char* fullResponseBuffer = new char[item->responseSize];

		// Copy the memory from the array version of the response string to the buffer. Use the data
		// method instead of the c_str method so it works with binary data, which may contain the null
		// terminator anywhere. Because it's not an ordinary null-terminated C string, it's necessary to
		// keep track of the length (hence the responseSize field).
		memcpy(fullResponseBuffer, item->response.data(), item->responseSize);

		//Sử dụng để gửi xuống client
		int fullLength = item->responseSize;
		int bytesSent = 0;
		int bytesLeft = fullLength;
		int r = -1;


		// TODO: Send response headers first. If sending a big file, this is good for the client so it knows how large the file is and can display a progress bar.


		// While the response has not been fully sent (large files won't be sent all at once)
		// https://beej.us/guide/bgnet/output/html/multipage/advanced.html#sendall
		while (bytesSent < fullLength) {
			// Send the server's response to the client
			r = send(SocketAccept, fullResponseBuffer + bytesSent, bytesLeft, 0);

			if (r == -1) {
				perror("send() failed");
				exit(EXIT_FAILURE);
			}

			bytesSent += r;
			bytesLeft -= r;
		}

		// Close the client's socket file descriptor
		if (closesocket(SocketAccept) == -1) {
			perror("close() failed");
			exit(EXIT_FAILURE);
		}


		delete[] fullResponseBuffer;
		::WaitForSingleObject(pChildThread->m_hThread, 20000);
	
		return 0;
	}

	//------ Tạo Socket kết nối đến server
	PX.URL = URL;
	PX.withServer = (TryConnectServer(s));

	if (PX.withServer == NULL)
	{
		closesocket(SocketAccept);
		return 1;
	}

	//gửi theo version 1.0
	if (result == 10)
		return ConnectVersion1_0(&PX,Buf,Rec);
	//Theo phiên bản 1.1

	//Bắt kết nối giữa luồng gởi và luồng nhận cùng lúc
/*	if (send(PX.withServer, Buf, Rec, 0) == -1) {
		perror("send() failed");
		exit(EXIT_FAILURE);
	}*/
	pChildThread = AfxBeginThread(ConnectServer, (LPVOID)&PX);
	
	int iResult;
	do
		{
		iResult = send(PX.withServer, Buf, Rec, 0);
		if (iResult == SOCKET_ERROR)
		{
			wprintf(L"send failed with error: %d\n", WSAGetLastError());
			//	closesocket(PX.withServer);
			break;
		}

		iResult = recv(SocketAccept, Buf, BUFSIZE, 0);
		Rec = iResult;
		if (iResult == 0)
		{
			wprintf(L"Connection closed\n");
			break;
		}
		else if (iResult < 0)
		{
			wprintf(L"recv failed with error: %d\n", WSAGetLastError());
		}

	} while (iResult > 0);

	//Đóng các socket
	closesocket(PX.withServer);
	closesocket(SocketAccept);
	//*/
	::WaitForSingleObject(pChildThread->m_hThread, 20000);
	return 0;
}

//Luồng nhận ở server và gửi xuống client
UINT ConnectServer(LPVOID pParam)
{
	PROXY* PX = (PROXY*)pParam;
	const int responseBufferSize = 131072;

	string fullResponse;

	int fullResponseSize = 0;
	char responseBuffer[responseBufferSize];

	char Buf[131072];
	int iResult,Rec;
	do
	{
		iResult = recv(PX->withServer, Buf, 131072, 0);
		Rec = iResult;
		//Nếu nhận không kb tức là kết nối đã đóng
		if (iResult == 0)
		{
			wprintf(L"Connection closed\n");
			break;
		}
		//Nhỏ hơn 0 là lỗi
		else if (iResult < 0)
		{
			wprintf(L"recv failed with error: %d\n", WSAGetLastError());
			break;
		}

		fullResponse.append(Buf, iResult);

		fullResponseSize += iResult;

		//Gửi gói dữ liệu vừa nhận xuống client
		iResult = send(PX->withClient, Buf, Rec, 0);
		if (iResult == SOCKET_ERROR)
		{
			wprintf(L"send failed with error: %d\n", WSAGetLastError());
			break;
		}
	} while (iResult > 0);
	closesocket(PX->withServer);
	closesocket(PX->withClient);

	//Fix URL, ERROR dont have url for item
	CacheItem* item = new CacheItem(PX->URL, fullResponse, fullResponseSize);
	
	// Cache the response
	cache->insert(item);
	cout << "Push URL: " << PX->URL << " into cache" << endl;
	return 0;
}

//Tạo cổng kết nối tới server
SOCKET TryConnectServer(string s)
{
	struct hostent *host;
	int iplen = 16;
	char ip[16];
	host = gethostbyname(s.c_str());
	if (host == NULL)
	{
		wprintf(L"Can't get IP\n");
		return NULL;
	}
	if (inet_ntop(AF_INET, (void *)host->h_addr_list[0], ip, iplen) == NULL)
	{
		perror("Can't resolve host");
	//	exit(1);
		return NULL;
	}
	/*else
	{
		cout << s << endl;
		cout << ip << endl;
	}*/

	//Khởi tạo socket kết nối tới server
	SOCKET sockServer;
	sockaddr_in Server;
	int iResult;
	sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	Server.sin_family = AF_INET;
	Server.sin_addr.s_addr = inet_addr(ip);
	Server.sin_port = htons(PORTSERVER);
	
	//Tạo kết nối tới server
	iResult = connect(sockServer, (SOCKADDR *)& Server, sizeof(Server));
	if (iResult == SOCKET_ERROR)
	{
		wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
		iResult = closesocket(sockServer);
		if (iResult == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
		return NULL;
	}

	//-----Trả về để xử lí kết nối từ socketAccept đến socketServer ---

	return sockServer;
}

//Dùng cho phiên bản 1.0
UINT ConnectVersion1_0(PROXY *PX,char Buf[],int Rec)
{
	int iResult;
	iResult = send(PX->withServer, Buf, Rec, 0);
	if (iResult == SOCKET_ERROR)
	{
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		//	closesocket(PX.withServer);
		return 1;
	}
	iResult = recv(PX->withServer, Buf, BUFSIZE, 0);
	Rec = iResult;
	//Nếu nhận không kb tức là kết nối đã đóng
	if (iResult == 0)
	{
		wprintf(L"Connection closed\n");
		return 2;
	}
	//Nhỏ hơn 0 là lỗi
	else if (iResult < 0)
	{
		wprintf(L"recv failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	iResult = send(PX->withClient, Buf, Rec, 0);
	if (iResult == SOCKET_ERROR)
	{
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	closesocket(PX->withServer);
	closesocket(PX->withClient);
	return 0;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// try to initialize MFC 
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		cerr << ("MFC failed to initialize!") << endl;
		nRetCode = 1;
	}
	else
	{
		//Lấy danh sách các file bị ban
		string line;
	
		ifstream  f;
		f.open("blacklist.conf");
		while (!f.eof())
		{
			getline(f,line);
			webBanned = webBanned + " " +line;
		}
		f.close();


		//Khởi tạo win Winsock
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		if  ((WSAStartup(wVersionRequested, &wsaData)) != 0)
		{
			printf("We could not find a usable ");
		}
		else
		{
			StartUpProxy();
		//	closesocket(MainProxy);
		}
		//WSACleanup();
	}

	return nRetCode;
}
