// tcp_server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../ImaysNet/ImaysNet.h"
#include <cstdlib> 

using namespace std;

#pragma pack(push,4)
typedef struct {
	float m_Transform[3];
	float m_Rotation[3];
	int m_TankId;
	float m_Health;

}socket_data;



int main()
{

	try
	{
		Endpoint ep = Endpoint("0.0.0.0", 5555);


		Socket listenSocket(SocketType::Udp);		// 1

		listenSocket.Bind(ep);		// 2

		//listenSocket.Listen();					// 3
		cout << "Server started.\n";



		// 4
		//Socket udpConnection;
		//string ignore;
		//listenSocket.Accept(udpConnection, ignore);



		// 5
	//	auto a = udpConnection.GetPeerAddr().ToString();
	//	cout << "Socket from " << a << " is accepted.\n";

		socket_data m_Movement;
		const int buf_size = 32;
		const int buf_num = 16;
		unsigned char buffer[buf_num][buf_size];

		for (int i = 0; i < buf_num; i++)
		{
			memset(&buffer[i], 0, buf_size);
		}

		int bit = 0;

		while (true)
		{
			// 6

		//	cout << "Receiving data...\n";




			listenSocket.ReceiveFrom(ep);

			memcpy(&m_Movement, listenSocket.m_unityBufferReceive, sizeof(m_Movement)); // => m_data에 복사해보자

			// 수신된 데이터를 꺼내서 출력합니다. 송신자는 "hello\0"을 보냈으므로 sz가 있음을 그냥 믿고 출력합니다.
			// (실전에서는 클라이언트가 보낸 데이터는 그냥 믿으면 안됩니다. 그러나 여기는 독자의 이해를 위해 예외로 두겠습니다.)


			int id = m_Movement.m_TankId;

			memcpy(buffer[id], &m_Movement, sizeof(socket_data));

			int ran = std::rand() % 4;
			int ran2 = std::rand() % 4;
			int ran3 = std::rand() % 4;

			while (ran % 4 == id % 4) ran = std::rand() % 4;


			id /= 4;

			id *= 4;

			memcpy(&listenSocket.m_unityBufferSend, &buffer[id + ran], sizeof(socket_data));

			listenSocket.SendTo(ep);

			/*
			for (int i = 0; i < 3; i++)
			{
				cout << m_data[m_Movement.m_TankId].m_Transform[i] << endl;
			}

			for (int i = 0; i < 3; i++)
			{
				cout << m_data[m_Movement.m_TankId].m_Rotation[i] << endl;
			}

			cout << m_data[m_Movement.m_TankId].m_TankId << endl;
			cout << m_data[m_Movement.m_TankId].m_Health << endl;


			*/

			/*

			for (int i = 0; i < m_UserNumber; i++)
			{

				if (i == m_Movement.m_TankId) continue;

				if (i == 0) memcpy(listenSocket.m_unityBufferSend, buffer0, sizeof(buffer0));
				else if (i == 1) memcpy(listenSocket.m_unityBufferSend, buffer1, sizeof(buffer1));
				else if (i == 2) memcpy(listenSocket.m_unityBufferSend, buffer2, sizeof(buffer2));
				else if (i == 3) memcpy(listenSocket.m_unityBufferSend, buffer3, sizeof(buffer3));

				//cout << "Sending data...\n";

				listenSocket.SendTo(ep);



			}

		*/

		/*

		for (int i = 0; i < 4; i++)
		{
			cout << check[i] << " ";
		}
		cout << endl;

		*/


		}
		listenSocket.Close();            // 8
	}
	catch (Exception& e)
	{
		cout << "Exception! " << e.what() << endl;
	}
	return 0;
}

