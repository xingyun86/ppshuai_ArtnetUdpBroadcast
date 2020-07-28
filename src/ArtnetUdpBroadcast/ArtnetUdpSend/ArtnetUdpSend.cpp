// ArtnetUdpSend.cpp : Defines the entry point for the application.
//

#include "ArtnetUdpSend.h"

//UDPSendBroadcast.cpp
#include <winsock2.h>
#include <thread>
#include <vector>

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

#ifdef _MSC_VER
#define NETWORK_STARTUP() WSADATA wsaData = { 0 }; WSAStartup(MAKEWORD(2, 2), &wsaData)
#define NETWORK_CLEANUP() WSACleanup()
#else
#define NETWORK_STARTUP()
#define NETWORK_CLEANUP()
#endif

void print_header(ArtNetDmx512Hdr* pkgHdr)
{
	std::cout << "ID:" << pkgHdr->ID << std::endl;
	std::cout << "OpCode:" << std::hex <<pkgHdr->OpCode << std::endl;
	std::cout << "ProtVerHi:" << std::hex << pkgHdr->ProtVerHi << std::endl;
	std::cout << "ProtVerLo:" << std::hex << pkgHdr->ProtVerLo << std::endl;
	std::cout << "Sequence:" << std::hex << pkgHdr->Sequence << std::endl;
	std::cout << "Physical:" << std::hex << pkgHdr->Physical << std::endl;
	std::cout << "DataLength:" << std::hex << ntohs(pkgHdr->DataLength) << std::endl;
}

void packet_header(
	ArtNetDmx512Hdr* pkgHdr,
	uint16_t opCode, 
	uint16_t protVerHi, 
	uint16_t protVerLo, 
	uint8_t sequence, 
	uint8_t physical, 
	uint16_t dataLen
)
{
	////////////////////////////////////////
	// Package Art-Net data
	//ArtNetDmx512Hdr pkg;
	// set ID
	memcpy(&pkgHdr->ID, "Art-Net\0", 8);
	// set OpCode
	pkgHdr->OpCode = opCode;
	// set ProtVerHi 
	pkgHdr->ProtVerHi = protVerHi;
	// set ProtVerLo 
	pkgHdr->ProtVerLo = protVerLo;
	// set Sequence
	pkgHdr->Sequence = sequence;
	// set Physical
	pkgHdr->Physical = physical;
	// set DataLength
	pkgHdr->DataLength = htons(dataLen);
}

uint8_t* create_dmxcmd_packet(ArtNetDmx512Hdr* pkgHdr)
{
	uint8_t * pkg = new uint8_t[sizeof(ArtNetDmx512Hdr) + pkgHdr->DataLength]();
	if (pkg == nullptr)
	{
		return nullptr;
	}
	memcpy(pkg, pkgHdr, sizeof(ArtNetDmx512Hdr));
}
int set_dmx_channel_value(uint8_t* pkg, uint16_t pos, uint8_t val)
{
	if (pos >= ((ArtNetDmx512Hdr *)pkg)->DataLength)
	{
		return (-1);
	}
	pkg[sizeof(ArtNetDmx512Hdr) + pos] = val;
	return 0;
}
void delete_packet(uint8_t* pkg)
{
	if (pkg != nullptr)
	{
		delete pkg;
	}
}

void do_send_broadcast(const char * ip)
{
	SOCKET SendSocket;
	sockaddr_in toAddr;//服务器地址
	int toAddrSize = sizeof(toAddr);
	int Port = 0x1936;//服务器监听地址
	//创建Socket对象
	SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	const int optval = 1;
	setsockopt(SendSocket, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval)); //设置套接字选项
	setsockopt(SendSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)); //设置地址复用选项

	//设置服务器地址
	sockaddr_in nameAddr;//服务器地址
	int nameAddrSize = sizeof(nameAddr);
	nameAddr.sin_family = AF_INET;
	nameAddr.sin_port = htons(Port);
	nameAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//多网卡下需要指定网卡IP进行广播
	//nameAddr.sin_addr.s_addr = inet_addr("172.30.128.1");
	//nameAddr.sin_addr.s_addr = inet_addr("2.168.0.10");
	nameAddr.sin_addr.s_addr = inet_addr(ip);
	bind(SendSocket, (const sockaddr*)&nameAddr, nameAddrSize);

	toAddr.sin_family = AF_INET;
	toAddr.sin_port = htons(Port);
	toAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);//套接字地址为广播地址
	//toAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
	//向服务器发送数据报
	printf("Sending a datagram to the receiver...\n");

	////////////////////////////////////////
	// Package Art-Net data
	uint16_t dataLength = 0x48;
	ArtNetDmx512Hdr pkgHdr;
	packet_header(&pkgHdr, 0x2400, 0x00, 0x0E, 0x05, 0xEF, dataLength);
	uint8_t* pkg = create_dmxcmd_packet(&pkgHdr);
	uint16_t pkg_len = sizeof(pkgHdr) + ntohs(pkgHdr.DataLength);
	set_dmx_channel_value(pkg, 0, 0x2a);
	set_dmx_channel_value(pkg, 1, 0x00);
	set_dmx_channel_value(pkg, 2, 0x0a);
	set_dmx_channel_value(pkg, 3, 0x81);
	set_dmx_channel_value(pkg, 4, 0x2a);
	set_dmx_channel_value(pkg, 5, 0x02);
	set_dmx_channel_value(pkg, 6, 0x00);
	set_dmx_channel_value(pkg, 7, 0x00);
	set_dmx_channel_value(pkg, 8, 0x23);
	set_dmx_channel_value(pkg, 9, 0x4b);
	set_dmx_channel_value(pkg, 10, 0x7e);
	set_dmx_channel_value(pkg, 11, 0x02);

	int i = 0;
	int iMax = 10;
	while (i++ < iMax)
	{
		int sendBytes = sendto(SendSocket, (const char*)pkg, pkg_len, 0, (sockaddr*)&toAddr, toAddrSize);
		printf("sendto [%d] packet bytes=%d!\n", i, sendBytes);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	delete[] pkg;

	//发送完成，关闭Socket
	printf("finished sending,close socket.\n");
	closesocket(SendSocket);
	printf("Exting.\n");
}

int run()
{
	char hostname[1024] = { 0 };
	std::vector<std::thread> task_list;
	gethostname(hostname, sizeof(hostname));    //获得本地主机名
	struct hostent * hostinfo = gethostbyname(hostname);//信息结构体
	if (hostinfo != nullptr)
	{
		while ((hostinfo->h_addr_list != nullptr) && *(hostinfo->h_addr_list) != nullptr) {
			char* ip = inet_ntoa(*(struct in_addr*)(*hostinfo->h_addr_list));
			std::cout << "ip=" << ip << std::endl;
			task_list.push_back(std::move(std::thread([](void* p)
				{
					struct in_addr _in_addr = { 0 };
					_in_addr.s_addr = (unsigned long)p;
					do_send_broadcast(inet_ntoa(_in_addr));
				}, (void *)((struct in_addr*)(*hostinfo->h_addr_list))->s_addr)
			));
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			hostinfo->h_addr_list++;
		}
		for (auto & it : task_list)
		{
			if (it.joinable())
			{
				it.join();
			}
		}
	}
	return 0;
}

int main(int argc, char ** argv)
{
	NETWORK_STARTUP();

	run();

	NETWORK_CLEANUP();
	return 0;
}
