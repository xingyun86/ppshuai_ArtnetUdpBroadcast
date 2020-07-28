// ArtnetUdpRecv.cpp : Defines the entry point for the application.
//

#include "ArtnetUdpRecv.h"

#include<winsock2.h>
#include <thread>
#include <vector>

#ifdef _MSC_VER
#define NETWORK_STARTUP() WSADATA wsaData = { 0 }; WSAStartup(MAKEWORD(2, 2), &wsaData)
#define NETWORK_CLEANUP() WSACleanup()
#else
#define NETWORK_STARTUP()
#define NETWORK_CLEANUP()
#endif

#pragma pack(1)
typedef struct tagArtNetDmx512Hdr {
	uint8_t ID[8];
	uint16_t OpCode;
	uint8_t ProtVerHi;
	uint8_t ProtVerLo;
	uint8_t Sequence;
	uint8_t Physical;
	uint16_t DataLength;
}ArtNetDmx512Hdr;
#pragma pack()

void print_header(ArtNetDmx512Hdr* pkgHdr)
{
	std::cout << "ID:" << pkgHdr->ID << std::endl;
	std::cout << "OpCode:" << std::hex << pkgHdr->OpCode << std::endl;
	std::cout << "ProtVerHi:" << std::hex << pkgHdr->ProtVerHi << std::endl;
	std::cout << "ProtVerLo:" << std::hex << pkgHdr->ProtVerLo << std::endl;
	std::cout << "Sequence:" << std::hex << pkgHdr->Sequence << std::endl;
	std::cout << "Physical:" << std::hex << pkgHdr->Physical << std::endl;
	std::cout << "DataLength:" << std::hex << ntohs(pkgHdr->DataLength) << std::endl;
}

void do_recv_broadcast(const char* ip)
{
	SOCKET recvSocket;
	sockaddr_in fromAddr;//服务器地址
	int fromAddrSize = sizeof(fromAddr);
	int Port = 0x1936;//服务器监听地址
	char SendBuf[1024];//发送数据的缓冲区
	int BufLen = 1024;//缓冲区大小

	//创建Socket对象
	recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	const int optval = 1;
	//setsockopt(recvSocket, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval)); //设置套接字选项
	setsockopt(recvSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)); //设置地址复用选项
	//设置服务器地址
	sockaddr_in nameAddr;//服务器地址
	int nameAddrSize = sizeof(nameAddr);
	nameAddr.sin_family = AF_INET;
	nameAddr.sin_port = htons(Port);
	//nameAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//nameAddr.sin_addr.s_addr = inet_addr("172.30.143.211");
	nameAddr.sin_addr.s_addr = inet_addr(ip);
	bind(recvSocket, (const sockaddr*)&nameAddr, nameAddrSize);

	//从服务器接收数据报
	printf("Recving a datagram from the sender...\n");

	////////////////////////////////////////
	// Package Art-Net data

	int i = 0;
	int iMax = 100;
	while (i++ < iMax)
	{
		uint16_t recv_size = 1024;
		uint8_t* recv_data = new uint8_t[recv_size]();
		int recvBytes = recvfrom(recvSocket, (char*)recv_data, recv_size, 0, (sockaddr*)&fromAddr, &fromAddrSize);
		printf("recvfrom [%d] packet bytes=%d!\n", i, recvBytes);
		print_header((ArtNetDmx512Hdr *)recv_data);
		delete[] recv_data;
		std::this_thread::sleep_for(std::chrono::microseconds(1000));
	}
	//发送完成，关闭Socket
	printf("finished recving,close socket.\n");
	closesocket(recvSocket);
	printf("Exting.\n");
}
int run()
{
	char hostname[1024] = { 0 };
	std::vector<std::thread> task_list;
	gethostname(hostname, sizeof(hostname));    //获得本地主机名
	struct hostent* hostinfo = gethostbyname(hostname);//信息结构体
	if (hostinfo != nullptr)
	{
		while ((hostinfo->h_addr_list != nullptr) && *(hostinfo->h_addr_list) != nullptr) {
			char* ip = inet_ntoa(*(struct in_addr*)(*hostinfo->h_addr_list));
			std::cout << "ip=" << ip << std::endl;
			task_list.push_back(std::move(std::thread([](void* p)
				{
					struct in_addr _in_addr = { 0 };
					_in_addr.s_addr = (unsigned long)p;
					do_recv_broadcast(inet_ntoa(_in_addr));
				}, (void*)((struct in_addr*)(*hostinfo->h_addr_list))->s_addr)
			));
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			hostinfo->h_addr_list++;
		}
		for (auto& it : task_list)
		{
			if (it.joinable())
			{
				it.join();
			}
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	NETWORK_STARTUP();	

	run();

	NETWORK_CLEANUP();
	return 0;
}
