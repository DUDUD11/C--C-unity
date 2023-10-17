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

// true가 되면 프로그램을 종료합니다.
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

// TCP 연결 각각의 객체.
class RemoteClient
{
public:
	shared_ptr<thread> thread; // 클라이언트 처리를 하는 스레드 1개
	Socket tcpConnection;		// accept한 TCP 연결
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
	// 에러 혹은 소켓 종료이다.
	// 해당 소켓은 제거해버리자. 
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
	// 사용자가 ctl-c를 누르면 메인루프를 종료하게 만듭니다.
	signal(SIGINT, ProcessSignalAction);

	memset(&m_datashell, 0, sizeof(m_datashell));
	memset(&m_dataredis, 0, sizeof(m_dataredis));


	auto lobby_renewal = chrono::system_clock::now();


	
	for (int i = 0; i < room_size * room_num; i++)
	{
		memset(&lobby_clients[i], 0, sizeof(RemoteClient));

	}

//	memset(&lobby_clients, 0, sizeof(lobby_clients));
	

	//redis 영역
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
		// IOCP를 써서 많은 수의 클라이언트를 받아 처리한다.

		// 많은 수의 클라가 tcp 5555로 연결 들어오고,
		// 그들은 hello world를 열심히 보낼 것이다.
		// 그것을 그냥 에코해주도록 하자.
		// 서버에서는 총 처리한 바이트수를 지속적으로 출력만 하자.

		// 성능보다는 소스 가독성을 우선으로 만들어져 있습니다. 
		// 가령 string 객체의 로컬변수 생성,삭제가 잦는 등의
		// 성능상 문제되는 부분은 알아서 개선하고 싶으시면 개선하십시오.



		// IOCP를 준비한다.
		Iocp iocp(1); // 본 예제는 스레드를 딱 하나만 쓴다. 따라서 여기도 1이 들어간다.

		// TCP 연결을 받는 소켓
		Socket listenSocket(SocketType::Tcp);
		listenSocket.Bind(Endpoint("0.0.0.0", 6200));

		listenSocket.Listen();

		// IOCP에 추가한다.
		iocp.Add(listenSocket, nullptr);

		// overlapped accept를 걸어둔다. 


		auto remoteClientCandidate = make_shared<RemoteClient>(SocketType::Tcp);

		string errorText;
		if (!listenSocket.AcceptOverlapped(remoteClientCandidate->tcpConnection, errorText)
			&& WSAGetLastError() != ERROR_IO_PENDING)
		{

			throw Exception("Overlapped AcceptEx failed.");
		}
		listenSocket.m_isReadOverlapped = true;

		cout << "서버가 시작되었습니다.\n";
		cout << "CTL-C키를 누르면 프로그램을 종료합니다.\n";

		// 리슨 소켓과 TCP 연결 소켓 모두에 대해서 I/O 가능(avail) 이벤트가 있을 때까지 기다린다. 
		// 그리고 나서 I/O 가능 소켓에 대해서 일을 한다.

		while (!stopWorking)
		{
			// I/O 완료 이벤트가 있을 때까지 기다립니다.
			IocpEvents readEvents;

			iocp.Wait(readEvents, 100);

			// 받은 이벤트 각각을 처리합니다.
			for (int i = 0; i < readEvents.m_eventCount; i++)
			{

				auto& readEvent = readEvents.m_events[i];
				if (readEvent.lpCompletionKey == 0) // 리슨소켓이면
				{
					listenSocket.m_isReadOverlapped = false;
					// 이미 accept은 완료되었다. 귀찮지만, Win32 AcceptEx 사용법에 따르는 마무리 작업을 하자. 
					if (remoteClientCandidate->tcpConnection.UpdateAcceptContext(listenSocket) != 0)
					{
						//리슨소켓을 닫았던지 하면 여기서 에러날거다. 그러면 리슨소켓 불능상태로 만들자.
						listenSocket.Close();
					}
					else // 잘 처리함
					{
						shared_ptr<RemoteClient> remoteClient = remoteClientCandidate;

						// 새 TCP 소켓도 IOCP에 추가한다.
						iocp.Add(remoteClient->tcpConnection, remoteClient.get());

						// overlapped 수신을 받을 수 있어야 하므로 여기서 I/O 수신 요청을 걸어둔다.
						if (remoteClient->tcpConnection.ReceiveOverlapped() != 0
							&& WSAGetLastError() != ERROR_IO_PENDING)
						{
							// 에러. 소켓을 정리하자. 그리고 그냥 버리자.
							remoteClient->tcpConnection.Close();
						}
						else
						{
							// I/O를 걸었다. 완료를 대기하는 중 상태로 바꾸자.
							remoteClient->tcpConnection.m_isReadOverlapped = true;

							// 새 클라이언트를 목록에 추가.
							remoteClients.insert({ remoteClient.get(), remoteClient });

							cout << "Client joined. There are " << remoteClients.size() << " connections.\n";
						}



						// 계속해서 소켓 받기를 해야 하므로 리슨소켓도 overlapped I/O를 걸자.
						remoteClientCandidate = make_shared<RemoteClient>(SocketType::Tcp);
						string errorText;
						if (!listenSocket.AcceptOverlapped(remoteClientCandidate->tcpConnection, errorText)
							&& WSAGetLastError() != ERROR_IO_PENDING)
						{
							// 에러나면 리슨소켓 불능 상태로 남기자. 
							listenSocket.Close();
						}
						else
						{
							// 리슨소켓은 연결이 들어옴을 기다리는 상태가 되었다.
							listenSocket.m_isReadOverlapped = true;
						}


					}
				}
				else  // TCP 연결 소켓이면
				{
					// 받은 데이터를 그대로 회신한다.
					shared_ptr<RemoteClient> remoteClient = remoteClients[(RemoteClient*)readEvent.lpCompletionKey];

					if (readEvent.dwNumberOfBytesTransferred <= 0)
					{
						int a = 0;
					}


					if (remoteClient)
					{
						// 이미 수신된 상태이다. 수신 완료된 것을 그냥 꺼내 쓰자.
						remoteClient->tcpConnection.m_isReadOverlapped = false;
						int ec = readEvent.dwNumberOfBytesTransferred;

						if (ec <= 0)
						{



							// 읽은 결과가 0 즉 TCP 연결이 끝났다...
							// 혹은 음수 즉 뭔가 에러가 난 상태이다...
							ProcessClientLeave(remoteClient);

							// 찾아서삭제

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
							// 이미 수신된 상태이다. 수신 완료된 것을 그냥 꺼내 쓰자.
							unsigned char* echoData = remoteClient->tcpConnection.m_shellreceiveBuffer;
							int echoDataLength = ec;

							// 원칙대로라면 TCP 스트림 특성상 일부만 송신하고 리턴하는 경우도 고려해야 하나,
							// 지금은 독자의 이해가 우선이므로, 생략하도록 한다.
							// 원칙대로라면 여기서 overlapped 송신을 해야 하지만 
							// 독자의 이해를 위해서 그냥 블로킹 송신을 한다.

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
							//// 통신 type
							
							수신
							1) 포탄
							2) 로비에서 방클릭했을때
							3) 로비에 들어왔을때
							


							송신

							- ) 포탄 3에서 만든 map 이용해서 포탄 패킷 보내야할듯

							1) 로비 대기열 수락이 가능한 경우
							=> 게임시작이 가능한 경우 3으로 넘어감

							2) 로비 대기열 수락이 불가능한 경우 (이미 존재할때)

							3) 게임 시작을 client에 송신코드

							4) 로비는 몇초마다 10초마다 갱신해서 보내주자..

							서버 콘솔 제거할때 lobbyclient** 도 갱신.

							5) 이미시작해서 다시들어갈때

							disconnect 할때 room pointer 제거


							아직 미구현 게임종료할때 redis로 게임상태 갱신

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

										memcpy(m_lobbydata->name_buf + sz, reply->element[j]->str, reply->element[j]->len); // 되는지 확인할것

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


							//플레이중 포탄처리
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

							//로비 방클릭했을때
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
									m_tempredis.m_playing = false; // 4개 다차면 true로 바꾸고 게임시작 사인을 보내자.
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

										
										// point 찾아서 send 해야함.. 3을 보내자.. redis도 playing 바꿔줘야함
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
											tempbuffer.m_tankid = 0; // dummy 값
											tempbuffer.m_type = 3;

											memcpy(lobby_clients[i]->tcpConnection.m_shellreceiveBuffer, &tempbuffer, sizeof(tempbuffer));


											lobby_clients[i]->tcpConnection.Send(reinterpret_cast<char*>(lobby_clients[i]->tcpConnection.m_shellreceiveBuffer), sizeof(tempbuffer));



											// m_playing 값을 바꿔주자

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
							

							//10초마다 보내주기 처음접속햇을때도 보내줘야한다.

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

										//memcpy(ptr_val + sz, reply->element[j]->str, reply->element[j]->len); // 되는지 확인할것
										
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

											//id는 가지고있는걸로 하자

											tempbuffer.m_playing = true;
											tempbuffer.m_tankid = id; // redis 값 확인필요
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


							





							
							// 다시 수신을 받으려면 overlapped I/O를 걸어야 한다.
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
								// I/O를 걸었다. 완료를 대기하는 중 상태로 바꾸자.
								remoteClient->tcpConnection.m_isReadOverlapped = true;
							}
						}
					}
				}

				//인원수 채워지면 시작하기 위해 유저들에게 데이터를 보내자.

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

		// 사용자가 CTL-C를 눌러서 루프를 나갔다. 모든 종료를 하자.
		// 그러나 overlapped I/O가 모두 완료되기 전에는 그냥 무작정 나가면 안된다. 운영체제가 백그라운드로 overlapped I/O가 진행중이기 때문이다.
		// 완료를 모두 체크하고 나가도록 하자.
		redisFree(c);
		listenSocket.Close();
		for (auto i : remoteClients)
		{
			i.second->tcpConnection.Close();
		}

		cout << "서버를 종료하고 있습니다...\n";
		while (remoteClients.size() > 0 || listenSocket.m_isReadOverlapped)
		{
			// I/O completion이 없는 상태의 RemoteClient를 제거한다.
			for (auto i = remoteClients.begin(); i != remoteClients.end();)
			{
				if (!i->second->tcpConnection.m_isReadOverlapped)
				{
					i = remoteClients.erase(i);
				}
				else
					i++; // 좀 더 기다려보자.
			}

			// I/O completion이 발생하면 더 이상 Overlapped I/O를 걸지 말고 '이제 정리해도 돼...'를 플래깅한다.
			IocpEvents readEvents;
			iocp.Wait(readEvents, 100);

			// 받은 이벤트 각각을 처리합니다.
			for (int i = 0; i < readEvents.m_eventCount; i++)
			{
				auto& readEvent = readEvents.m_events[i];
				if (readEvent.lpCompletionKey == 0) // 리슨소켓이면
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

		cout << "서버 끝.\n";

	}
	catch (Exception& e)
	{
		cout << "Exception! " << e.what() << endl;
	}

	return 0;
}

