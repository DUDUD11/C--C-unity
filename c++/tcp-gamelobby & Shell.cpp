#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include "../ImaysNet/ImaysNet.h"
#include <stdio.h>
#include <signal.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <iostream>
#include <mutex>
#include <hiredis.h>




using namespace std;

// true�� �Ǹ� ���α׷��� �����մϴ�.
volatile bool stopWorking = false;
const int id_size = 20;
const int room_size = 4;
const int room_num = 4;

int buffToInteger(unsigned char* buffer)
{
	int a;
	memcpy(&a, buffer, sizeof(int));
	return a;
}

bool buffTOBool(char* buffer)
{
	bool b;
	memcpy(&b, buffer, sizeof(bool));
	return b;
}


int buffToInteger(char* buffer)
{
	char a;
	memcpy(&a, buffer, sizeof(char));
	return a-'0';
}




typedef struct {

	int m_type;
	float m_transform[3];
	float m_rotation[4];
	float m_velocity[3];
	int m_tankid;

}shell_data;

typedef struct {
	int32_t m_type;
	int32_t m_tankid;
	char m_id[id_size];
	bool m_playing;

}redis_data;

typedef struct {

	int m_type;
	char name_buf[500];

} lobby_name_data;


void ProcessSignalAction(int sig_number)
{
	if (sig_number == SIGINT)
		stopWorking = true;
}

// TCP ���� ������ ��ü.
class RemoteClient
{
public:
	shared_ptr<thread> thread; // Ŭ���̾�Ʈ ó���� �ϴ� ������ 1��
	Socket tcpConnection;		// accept�� TCP ����
	char m_id[id_size];

	RemoteClient() {}
	RemoteClient(SocketType socketType) :tcpConnection(socketType) {}
};

unordered_map<RemoteClient*, shared_ptr<RemoteClient>> remoteClients;
RemoteClient** lobby_clients = new RemoteClient* [room_num * room_size];
//unordered_set<RemoteClient*> GameReadyClients;
//recursive_mutex GameReadyClients_mutex;
//int idnum = 0;
//recursive_mutex idnum_mutex;
shell_data m_datashell;
redis_data m_dataredis;

void ProcessClientLeave(shared_ptr<RemoteClient> remoteClient)
{
	// ���� Ȥ�� ���� �����̴�.
	// �ش� ������ �����ع�����. 
	remoteClient->tcpConnection.Close();
	remoteClients.erase(remoteClient.get());

	/*

	lock_guard<recursive_mutex> GameReadyClients_lock(GameReadyClients_mutex);
	if ((GameReadyClients.erase(remoteClient.get()) != 0))
	{
		lock_guard<recursive_mutex> idnum_lock(idnum_mutex);
		idnum--;
	}

	*/

	cout << "Client left. There are " << remoteClients.size() << " connections.\n";
}


int main()
{
	// ����ڰ� ctl-c�� ������ ���η����� �����ϰ� ����ϴ�.
	signal(SIGINT, ProcessSignalAction);

	memset(&m_datashell, 0, sizeof(m_datashell));
	memset(&m_dataredis, 0, sizeof(m_dataredis));


	auto lobby_renewal = chrono::system_clock::now();


	
	for (int i = 0; i < room_size * room_num; i++)
	{
		memset(&lobby_clients[i], 0, sizeof(RemoteClient));

	}

//	memset(&lobby_clients, 0, sizeof(lobby_clients));
	

	//redis ����
	redisContext* c;
	redisReply* reply;



	struct timeval timeout = { 5, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout((char*)"127.0.0.1", 6379, timeout);
	if (c->err) {
		printf("Connection error: %s\n", c->errstr);
		exit(1);
	}

	// PING server 
	reply = (redisReply*)redisCommand(c, "PING");
	printf("PING: %s\n", reply->str);
	freeReplyObject(reply);

	/*

	// Set a key 
	reply = (redisReply*)redisCommand(c, "SET %s %s", "foo", "hello world");
	printf("SET: %s\n", reply->str);
	freeReplyObject(reply);

	// Set a key using binary safe API 
	reply = (redisReply*)redisCommand(c, "SET %b %b", "bar", 3, "hello", 5);
	printf("SET (binary API): %s\n", reply->str);
	freeReplyObject(reply);

	

	// Try a GET and two INCR 
	reply = (redisReply*)redisCommand(c, "GET foo");
	printf("GET foo: %s\n", reply->str);
	freeReplyObject(reply);

	reply = (redisReply*)redisCommand(c, "INCR counter");
	printf("INCR counter: %lld\n", reply->integer);
	freeReplyObject(reply);
	// again ... 
	reply = (redisReply*)redisCommand(c, "INCR counter");
	printf("INCR counter: %lld\n", reply->integer);
	freeReplyObject(reply);

	// Create a list of numbers, from 0 to 9 
	reply = (redisReply*)redisCommand(c, "DEL mylist");
	freeReplyObject(reply);
	for (unsigned int j = 0; j < 10; j++) {
		char buf[64];

		snprintf(buf, 64, "%d", j);
		reply = (redisReply*)redisCommand(c, "LPUSH mylist element-%s", buf);
		freeReplyObject(reply);
	}

	// Let's check what we have inside the list //
	reply = (redisReply*)redisCommand(c, "LRANGE mylist 0 -1");
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (unsigned int j = 0; j < reply->elements; j++) {
			printf("%u) %s\n", j, reply->element[j]->str);
		}
	}
	freeReplyObject(reply);

	*/



	try
	{
		// IOCP�� �Ἥ ���� ���� Ŭ���̾�Ʈ�� �޾� ó���Ѵ�.

		// ���� ���� Ŭ�� tcp 5555�� ���� ������,
		// �׵��� hello world�� ������ ���� ���̴�.
		// �װ��� �׳� �������ֵ��� ����.
		// ���������� �� ó���� ����Ʈ���� ���������� ��¸� ����.

		// ���ɺ��ٴ� �ҽ� �������� �켱���� ������� �ֽ��ϴ�. 
		// ���� string ��ü�� ���ú��� ����,������ ��� ����
		// ���ɻ� �����Ǵ� �κ��� �˾Ƽ� �����ϰ� �����ø� �����Ͻʽÿ�.



		// IOCP�� �غ��Ѵ�.
		Iocp iocp(1); // �� ������ �����带 �� �ϳ��� ����. ���� ���⵵ 1�� ����.

		// TCP ������ �޴� ����
		Socket listenSocket(SocketType::Tcp);
		listenSocket.Bind(Endpoint("0.0.0.0", 6200));

		listenSocket.Listen();

		// IOCP�� �߰��Ѵ�.
		iocp.Add(listenSocket, nullptr);

		// overlapped accept�� �ɾ�д�. 


		auto remoteClientCandidate = make_shared<RemoteClient>(SocketType::Tcp);

		string errorText;
		if (!listenSocket.AcceptOverlapped(remoteClientCandidate->tcpConnection, errorText)
			&& WSAGetLastError() != ERROR_IO_PENDING)
		{

			throw Exception("Overlapped AcceptEx failed.");
		}
		listenSocket.m_isReadOverlapped = true;

		cout << "������ ���۵Ǿ����ϴ�.\n";
		cout << "CTL-CŰ�� ������ ���α׷��� �����մϴ�.\n";

		// ���� ���ϰ� TCP ���� ���� ��ο� ���ؼ� I/O ����(avail) �̺�Ʈ�� ���� ������ ��ٸ���. 
		// �׸��� ���� I/O ���� ���Ͽ� ���ؼ� ���� �Ѵ�.

		while (!stopWorking)
		{
			// I/O �Ϸ� �̺�Ʈ�� ���� ������ ��ٸ��ϴ�.
			IocpEvents readEvents;

			iocp.Wait(readEvents, 100);

			// ���� �̺�Ʈ ������ ó���մϴ�.
			for (int i = 0; i < readEvents.m_eventCount; i++)
			{

				auto& readEvent = readEvents.m_events[i];
				if (readEvent.lpCompletionKey == 0) // ���������̸�
				{
					listenSocket.m_isReadOverlapped = false;
					// �̹� accept�� �Ϸ�Ǿ���. ��������, Win32 AcceptEx ������ ������ ������ �۾��� ����. 
					if (remoteClientCandidate->tcpConnection.UpdateAcceptContext(listenSocket) != 0)
					{
						//���������� �ݾҴ��� �ϸ� ���⼭ �������Ŵ�. �׷��� �������� �Ҵɻ��·� ������.
						listenSocket.Close();
					}
					else // �� ó����
					{
						shared_ptr<RemoteClient> remoteClient = remoteClientCandidate;

						// �� TCP ���ϵ� IOCP�� �߰��Ѵ�.
						iocp.Add(remoteClient->tcpConnection, remoteClient.get());

						// overlapped ������ ���� �� �־�� �ϹǷ� ���⼭ I/O ���� ��û�� �ɾ�д�.
						if (remoteClient->tcpConnection.ReceiveOverlapped() != 0
							&& WSAGetLastError() != ERROR_IO_PENDING)
						{
							// ����. ������ ��������. �׸��� �׳� ������.
							remoteClient->tcpConnection.Close();
						}
						else
						{
							// I/O�� �ɾ���. �ϷḦ ����ϴ� �� ���·� �ٲ���.
							remoteClient->tcpConnection.m_isReadOverlapped = true;

							// �� Ŭ���̾�Ʈ�� ��Ͽ� �߰�.
							remoteClients.insert({ remoteClient.get(), remoteClient });

							cout << "Client joined. There are " << remoteClients.size() << " connections.\n";
						}



						// ����ؼ� ���� �ޱ⸦ �ؾ� �ϹǷ� �������ϵ� overlapped I/O�� ����.
						remoteClientCandidate = make_shared<RemoteClient>(SocketType::Tcp);
						string errorText;
						if (!listenSocket.AcceptOverlapped(remoteClientCandidate->tcpConnection, errorText)
							&& WSAGetLastError() != ERROR_IO_PENDING)
						{
							// �������� �������� �Ҵ� ���·� ������. 
							listenSocket.Close();
						}
						else
						{
							// ���������� ������ ������ ��ٸ��� ���°� �Ǿ���.
							listenSocket.m_isReadOverlapped = true;
						}


					}
				}
				else  // TCP ���� �����̸�
				{
					// ���� �����͸� �״�� ȸ���Ѵ�.
					shared_ptr<RemoteClient> remoteClient = remoteClients[(RemoteClient*)readEvent.lpCompletionKey];

					if (readEvent.dwNumberOfBytesTransferred <= 0)
					{
						int a = 0;
					}


					if (remoteClient)
					{
						// �̹� ���ŵ� �����̴�. ���� �Ϸ�� ���� �׳� ���� ����.
						remoteClient->tcpConnection.m_isReadOverlapped = false;
						int ec = readEvent.dwNumberOfBytesTransferred;

						if (ec <= 0)
						{



							// ���� ����� 0 �� TCP ������ ������...
							// Ȥ�� ���� �� ���� ������ �� �����̴�...
							ProcessClientLeave(remoteClient);

							// ã�Ƽ�����

							for (int i = 0; i < room_num * room_size; i++)
							{
								if (remoteClient.get() == lobby_clients[i])
								{
									

									reply = (redisReply*)redisCommand(c, "DEL mylist.%s", &(lobby_clients[i]->m_id[0]));

									printf("DELETE mylist %s", reply->str);

									memset(&lobby_clients[i], 0, sizeof(RemoteClient));


									freeReplyObject(reply);




									break;
								}





							}


							

						}
						else
						{
							// �̹� ���ŵ� �����̴�. ���� �Ϸ�� ���� �׳� ���� ����.
							unsigned char* echoData = remoteClient->tcpConnection.m_shellreceiveBuffer;
							int echoDataLength = ec;

							// ��Ģ��ζ�� TCP ��Ʈ�� Ư���� �Ϻθ� �۽��ϰ� �����ϴ� ��쵵 ����ؾ� �ϳ�,
							// ������ ������ ���ذ� �켱�̹Ƿ�, �����ϵ��� �Ѵ�.
							// ��Ģ��ζ�� ���⼭ overlapped �۽��� �ؾ� ������ 
							// ������ ���ظ� ���ؼ� �׳� ���ŷ �۽��� �Ѵ�.

							int m_temp = buffToInteger(remoteClient->tcpConnection.m_shellreceiveBuffer);
							printf("%d\n", m_temp);
							if (m_temp == 5)
							{
								memcpy(&m_datashell, remoteClient->tcpConnection.m_shellreceiveBuffer, sizeof(m_datashell));
							}

							else
							{

								memcpy(&m_dataredis, remoteClient->tcpConnection.m_shellreceiveBuffer, sizeof(m_dataredis));
							}
					
							/*
							//// ��� type
							
							����
							1) ��ź
							2) �κ񿡼� ��Ŭ��������
							3) �κ� ��������
							


							�۽�

							- ) ��ź 3���� ���� map �̿��ؼ� ��ź ��Ŷ �������ҵ�

							1) �κ� ��⿭ ������ ������ ���
							=> ���ӽ����� ������ ��� 3���� �Ѿ

							2) �κ� ��⿭ ������ �Ұ����� ��� (�̹� �����Ҷ�)

							3) ���� ������ client�� �۽��ڵ�

							4) �κ�� ���ʸ��� 10�ʸ��� �����ؼ� ��������..

							���� �ܼ� �����Ҷ� lobbyclient** �� ����.

							5) �̹̽����ؼ� �ٽõ���

							disconnect �Ҷ� room pointer ����


							���� �̱��� ���������Ҷ� redis�� ���ӻ��� ����

							*/

							auto current_time = chrono::system_clock::now();
							auto sec = chrono::duration_cast<chrono::seconds>(current_time - lobby_renewal);


							if (sec.count() >= 10)
							{


								lobby_name_data* m_lobbydata = new lobby_name_data();
								memset(m_lobbydata->name_buf, 0, sizeof(m_lobbydata->name_buf) / sizeof(char));

								int sz = 0;

								m_lobbydata->m_type = 4;

								// Let's check what we have inside the list //
								reply = (redisReply*)redisCommand(c, "LRANGE mylist 0 -1");

								if (reply->type == REDIS_REPLY_ARRAY)
								{
									for (unsigned int j = 0; j < reply->elements; j++)
									{

										memcpy(m_lobbydata->name_buf + sz, reply->element[j]->str, reply->element[j]->len); // �Ǵ��� Ȯ���Ұ�

										sz += reply->element[j]->len + 1;

										if (reply->element[j]->len >= 1)
										{
											m_lobbydata->name_buf[sz - 1] = '\n';
										}

									}


									for (auto i : remoteClients)
									{
										memcpy(i.second->tcpConnection.m_shellreceiveBuffer, m_lobbydata, sizeof(lobby_name_data));

										i.second->tcpConnection.Send(reinterpret_cast<char*>(i.second->tcpConnection.m_shellreceiveBuffer), sizeof(lobby_name_data));
									}

								}


								freeReplyObject(reply);

								delete(m_lobbydata);

								lobby_renewal = chrono::system_clock::now();
							}


							//�÷����� ��źó��
							if (m_temp == 5)  
							{
								int cur = m_dataredis.m_tankid / 4;

								cur *= 4;

								for (int i = cur; i < cur + room_size; i++)
								{
									if (lobby_clients[i] == remoteClient.get()) continue;

							//		memcpy(remoteClient->tcpConnection.m_shellreceiveBuffer, &m_datashell+4, sizeof(m_datashell)-4);

									lobby_clients[i]->tcpConnection.Send(reinterpret_cast<char*>(&m_datashell), sizeof(shell_data));

								}


								
							}

							//�κ� ��Ŭ��������
							else if (m_temp == 2)
							{
							


								reply = (redisReply*)redisCommand(c, "GET %d",&m_dataredis.m_tankid);
								printf("GET %d: %s\n", m_dataredis.m_tankid, reply->str);

								redis_data m_data;
								strcpy(m_data.m_id, m_dataredis.m_id);
								m_data.m_playing = false;
								m_data.m_tankid = m_dataredis.m_tankid;
								m_data.m_type = 2;

								

								if (reply->str==NULL)
								{
									bool ready_flag = false;
									int flag_sum = 0;

									int cur = m_dataredis.m_tankid / 4;
									cur *= 4;

									for (int i = cur; i < cur + room_size; i++)
									{
										if (lobby_clients[i]!=NULL) flag_sum++;
									}

									redis_data m_tempredis;
									strcpy(m_tempredis.m_id, m_dataredis.m_id);
									m_tempredis.m_playing = false; // 4�� ������ true�� �ٲٰ� ���ӽ��� ������ ������.
									m_tempredis.m_tankid = m_dataredis.m_tankid;
									m_tempredis.m_type = 1;
									lobby_clients[m_dataredis.m_tankid] = remoteClient.get();


									memcpy(remoteClient->tcpConnection.m_shellreceiveBuffer, &m_tempredis, sizeof(m_tempredis));
									remoteClient->tcpConnection.Send(reinterpret_cast<char*>(remoteClient->tcpConnection.m_shellreceiveBuffer), sizeof(m_tempredis));

									strcpy(remoteClient->m_id, m_dataredis.m_id);


									string str;

									if (!m_tempredis.m_playing)
									{
										str = "0 " + to_string(m_tempredis.m_tankid);
									}

									else
									{
										str = "1 " + to_string(m_tempredis.m_tankid);
									}

									printf("%s\n", str);

									freeReplyObject(reply);

									reply = (redisReply*)redisCommand(c, "SET %s %s", &m_tempredis.m_id[0], &str);
									printf("SET: %s\n", reply->str);
									freeReplyObject(reply);

									


									
									if (flag_sum == 3)
									{
										m_tempredis.m_playing = true;
										m_tempredis.m_type = 3;

										
										// point ã�Ƽ� send �ؾ���.. 3�� ������.. redis�� playing �ٲ������
										for (int i = cur; i < cur + room_size; i++)

										{
											if (lobby_clients[i] == NULL)
											{
												cout << "lobby client null pointer error" << endl;
												return -1;
											}

											redis_data tempbuffer;

											strcpy(tempbuffer.m_id, lobby_clients[i]->m_id);
											tempbuffer.m_playing = true;
											tempbuffer.m_tankid = 0; // dummy ��
											tempbuffer.m_type = 3;

											memcpy(lobby_clients[i]->tcpConnection.m_shellreceiveBuffer, &tempbuffer, sizeof(tempbuffer));


											lobby_clients[i]->tcpConnection.Send(reinterpret_cast<char*>(lobby_clients[i]->tcpConnection.m_shellreceiveBuffer), sizeof(tempbuffer));



											// m_playing ���� �ٲ�����

											reply = (redisReply*)redisCommand(c, "GET %s", &(lobby_clients[i]->m_id[0]));
											printf("GET m_playing and tankid: %s\n", reply->str);

											reply->str[0] = '1';
									//		freeReplyObject(reply);

											reply = (redisReply*)redisCommand(c, "SET %s %s", lobby_clients[i]->m_id, reply->str);

											printf("SET: m_playing and tankid %s\n", reply->str);
											freeReplyObject(reply);

										}

									}





									for (auto i : remoteClients)
									{
										if (i.second.get() == remoteClient.get()) continue;

										memcpy(i.second->tcpConnection.m_shellreceiveBuffer, &m_data, sizeof(m_data));
										i.second->tcpConnection.Send(reinterpret_cast<char*>(i.second->tcpConnection.m_shellreceiveBuffer), sizeof(m_data));



								
									}


								}

								else
								{
	
									memcpy(remoteClient->tcpConnection.m_shellreceiveBuffer, &m_data, sizeof(m_data));

									remoteClient->tcpConnection.Send(reinterpret_cast<char*>(remoteClient->tcpConnection.m_shellreceiveBuffer), sizeof(m_data));

								}


							//	freeReplyObject(reply);


							//	reply = (redisReply*)redisCommand(c, "SET %s %s", "foo", "hello world");
							//	printf("SET: %s\n", reply->str);
							//	freeReplyObject(reply);

						


							}
							

							//10�ʸ��� �����ֱ� ó�������������� ��������Ѵ�.

							else if (m_temp == 3) {

								reply = (redisReply*)redisCommand(c, "RPUSH mylist %s", &m_dataredis.m_id[0]);

								printf("rpush mylist %s\n", reply->str);

								freeReplyObject(reply);


								lobby_name_data* m_lobbydata = new lobby_name_data();
								memset(m_lobbydata, 0, sizeof(lobby_name_data));

						

								m_lobbydata->m_type = 4;

								// Let's check what we have inside the list //
								reply = (redisReply*)redisCommand(c, "LRANGE mylist 0 -1");

								if (reply->type == REDIS_REPLY_ARRAY)
								{
									int sz = 0;
									char* ptr_val = m_lobbydata->name_buf;

									for (unsigned int j = 0; j < reply->elements; j++)
									{
										
										for (int lbd_ptr = 0; lbd_ptr < reply->element[j]->len; lbd_ptr++)
										{
											*(ptr_val + (sz++)) = reply->element[j]->str[lbd_ptr];

										}

										*(ptr_val + (sz++)) = '\n';

										//memcpy(ptr_val + sz, reply->element[j]->str, reply->element[j]->len); // �Ǵ��� Ȯ���Ұ�
										
										//sz += (reply->element[j]->len) + 1;

										//if (reply->element[j]->len >= 1)
										//{
										//	m_lobbydata->name_buf[sz - 1] = '\n';
										//}

									}

									//printf("%s", m_lobbydata->name_buf);
									memcpy(remoteClient->tcpConnection.m_shellreceiveBuffer, m_lobbydata, sizeof(lobby_name_data));
									remoteClient->tcpConnection.Send(reinterpret_cast<char*>(remoteClient->tcpConnection.m_shellreceiveBuffer), sizeof(lobby_name_data));

								}


								freeReplyObject(reply);

								delete(m_lobbydata);


									reply = (redisReply*)redisCommand(c, "GET %s", m_dataredis.m_id);

									if (reply->str != NULL )
									{
										printf("Game is Playing and re enter: %s\n", reply->str);

										if (buffTOBool(reply->str))
										{

											char* str_ptr = reply->str;
											str_ptr += 2;

											int id = buffToInteger(str_ptr);

											redis_data tempbuffer;

											//id�� �������ִ°ɷ� ����

											tempbuffer.m_playing = true;
											tempbuffer.m_tankid = id; // redis �� Ȯ���ʿ�
											tempbuffer.m_type = 50;

											cout << tempbuffer.m_playing+" " << tempbuffer.m_tankid+" " << tempbuffer.m_type << endl;

											memcpy(remoteClient->tcpConnection.m_shellreceiveBuffer, &tempbuffer, sizeof(redis_data));


											remoteClient->tcpConnection.Send(reinterpret_cast<char*>(remoteClient->tcpConnection.m_shellreceiveBuffer), sizeof(tempbuffer));
											


										}




									}

									freeReplyObject(reply);




							}


				

							else
							{
								throw Exception("Error occured m_temp value is wrong.");

							}


							





							
							// �ٽ� ������ �������� overlapped I/O�� �ɾ�� �Ѵ�.
							if (remoteClient->tcpConnection.ReceiveOverlapped() != 0
								&& WSAGetLastError() != ERROR_IO_PENDING)
							{
								ProcessClientLeave(remoteClient);


								for (int i = 0; i < room_num * room_size; i++)
								{
									cout << remoteClient.get() << std::hex << lobby_clients[i] << std::hex << endl;


									if (remoteClient.get() == lobby_clients[i])
									{
										

										reply = (redisReply*)redisCommand(c, "DEL mylist.%s", lobby_clients[i]->m_id);

										printf("DELETE mylist %s", reply->str);

										memset(&lobby_clients[i], 0, sizeof(RemoteClient));

										freeReplyObject(reply);


										break;
									}

								}

							}
							else
							{
								// I/O�� �ɾ���. �ϷḦ ����ϴ� �� ���·� �ٲ���.
								remoteClient->tcpConnection.m_isReadOverlapped = true;
							}
						}
					}
				}

				//�ο��� ä������ �����ϱ� ���� �����鿡�� �����͸� ������.

				/*

				if (idnum == m_UserNumber)
				{

					int seq = 0;

					for (auto i : remoteClients)
					{

						char tempbuffer[8];
						id_data m_tempstruct;
						m_tempstruct.m_Id = seq;
						m_tempstruct.m_ClientNumber = idnum;

						memcpy(tempbuffer, &m_tempstruct, sizeof(m_tempstruct));

						i.second->tcpConnection.Send(tempbuffer, sizeof(tempbuffer));

						seq++;
					}

				}

				*/


			}
		}

		// ����ڰ� CTL-C�� ������ ������ ������. ��� ���Ḧ ����.
		// �׷��� overlapped I/O�� ��� �Ϸ�Ǳ� ������ �׳� ������ ������ �ȵȴ�. �ü���� ��׶���� overlapped I/O�� �������̱� �����̴�.
		// �ϷḦ ��� üũ�ϰ� �������� ����.
		redisFree(c);
		listenSocket.Close();
		for (auto i : remoteClients)
		{
			i.second->tcpConnection.Close();
		}

		cout << "������ �����ϰ� �ֽ��ϴ�...\n";
		while (remoteClients.size() > 0 || listenSocket.m_isReadOverlapped)
		{
			// I/O completion�� ���� ������ RemoteClient�� �����Ѵ�.
			for (auto i = remoteClients.begin(); i != remoteClients.end();)
			{
				if (!i->second->tcpConnection.m_isReadOverlapped)
				{
					i = remoteClients.erase(i);
				}
				else
					i++; // �� �� ��ٷ�����.
			}

			// I/O completion�� �߻��ϸ� �� �̻� Overlapped I/O�� ���� ���� '���� �����ص� ��...'�� �÷����Ѵ�.
			IocpEvents readEvents;
			iocp.Wait(readEvents, 100);

			// ���� �̺�Ʈ ������ ó���մϴ�.
			for (int i = 0; i < readEvents.m_eventCount; i++)
			{
				auto& readEvent = readEvents.m_events[i];
				if (readEvent.lpCompletionKey == 0) // ���������̸�
				{
					listenSocket.m_isReadOverlapped = false;
				}
				else
				{
					shared_ptr<RemoteClient> remoteClient = remoteClients[(RemoteClient*)readEvent.lpCompletionKey];
					if (remoteClient)
					{
						remoteClient->tcpConnection.m_isReadOverlapped = false;
					}
				}
			}
		}

		cout << "���� ��.\n";

	}
	catch (Exception& e)
	{
		cout << "Exception! " << e.what() << endl;
	}

	return 0;
}

