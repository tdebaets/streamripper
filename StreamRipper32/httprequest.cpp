/*
Oddsock - StreamRipper32 (Win32 front end to streamripper library)
Copyright (C) 2000  Oddsock

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//#include <afxinet.h>
#include <winsock2.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

//#include <winsock2.h>




// Helper macro for displaying errors
#define PRINTERROR(s)	\
		fprintf(stderr,"\n%s %d\n", s, WSAGetLastError())

#define BUFFER_SIZE 8096


//CInternetSession *inetSession = new CInternetSession("Mozilla",1, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,0);


int GetHTTP(LPCSTR lpServerName, int Port, LPCSTR lpFileName, char *header, char *result)
{
	LPHOSTENT	lpHostEntry;
	SOCKADDR_IN saServer;
	SOCKET		Socket;

	int			nRet;
//	CStdioFile	*file;

	char		request[1024] = "";
	char		headers[255] = "";

//	sprintf(request, "http://yp.shoutcast.com/%s", lpFileName);
//	strcpy(request, "http://www.yahoo.com");

	/*
	file = inetSession->OpenURL(request, 0, INTERNET_FLAG_TRANSFER_ASCII, headers, strlen(headers));

	if (file == 0) {
		char	buf[255] = "";
		sprintf(buf, "Cannot open %s", request);
		MessageBox(NULL, buf, "Error", MB_OK);
		return -1;
	}
	*/
	//
	// Lookup the host address
	//

	lpHostEntry = gethostbyname(lpServerName);
	if (lpHostEntry == NULL)
	{
		PRINTERROR("socket()");
		return 0;
	}
	
	// Create a TCP/IP stream socket
	Socket = 0;
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		PRINTERROR("socket()");
		return 0;
	}

	//
	// Fill in the rest of the server address structure
	//
	saServer.sin_family = AF_INET;
	saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
	saServer.sin_port = htons(Port);

	//
	// Connect the socket
	//
	nRet = connect(Socket, (LPSOCKADDR)&saServer, sizeof(SOCKADDR_IN));
	if (nRet == SOCKET_ERROR)
	{
		PRINTERROR("connect()");
		closesocket(Socket);	
		return 0;
	}
	
	//
	// Format the HTTP request
	// and send it
	//
	char szBuffer[1024];
	sprintf(szBuffer, "GET %s HTTP/1.0\r\n%s\r\n\r\n", lpFileName, header);
	nRet = send(Socket, szBuffer, strlen(szBuffer), 0);
	if (nRet == SOCKET_ERROR)
	{
		PRINTERROR("send()");
		closesocket(Socket);	
		return 0;
	}

	char	*p1;

	p1 = result;
	int	loop = 1;
	while (loop) {
//		nRet = file->Read(p1, 255);
		nRet = recv(Socket, p1, 255, 0);
		if (nRet == 0) {
			loop = 0;
		}
		else {
			p1 = p1 + nRet;
		}
	}

	closesocket(Socket);	
//	file->Close();
	return 1;
}

