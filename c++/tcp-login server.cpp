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


#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>


using namespace std;
using namespace sql;

typedef struct {

	int m_type;
	unsigned char m_id[20];
	unsigned char m_pwd[20];


}id_data;

typedef struct {

	int m_val;

} sign_status;

// TCP 연결 각각의 객체.
class RemoteClient
{
public:
	shared_ptr<thread> thread; // 클라이언트 처리를 하는 스레드 1개
	Socket tcpConnection;		// accept한 TCP 연결

	RemoteClient() {}
	RemoteClient(SocketType socketType) :tcpConnection(socketType) {}
};





const string server = "tcp://127.0.0.1:3306";
const string username = "root";
const string password = "password";
volatile bool stopWorking = false;
unordered_map<RemoteClient*, shared_ptr<RemoteClient>> remoteClients;
unordered_set<RemoteClient*> GameReadyClients;
recursive_mutex GameReadyClients_mutex;
int idnum = 0;
recursive_mutex idnum_mutex;
id_data m_datastruct;


void ProcessSignalAction(int sig_number)
{
	if (sig_number == SIGINT)
		stopWorking = true;
}




void ProcessClientLeave(shared_ptr<RemoteClient> remoteClient)
{
	// 에러 혹은 소켓 종료이다.
	// 해당 소켓은 제거해버리자. 
	remoteClient->tcpConnection.Close();
	remoteClients.erase(remoteClient.get());
	lock_guard<recursive_mutex> GameReadyClients_lock(GameReadyClients_mutex);
	if ((GameReadyClients.erase(remoteClient.get()) != 0))
	{
		lock_guard<recursive_mutex> idnum_lock(idnum_mutex);
		idnum--;
	}

	cout << "Client left. There are " << remoteClients.size() << " connections.\n";
}

// true가 되면 프로그램을 종료합니다.



int main()
{
	// 사용자가 ctl-c를 누르면 메인루프를 종료하게 만듭니다.
	signal(SIGINT, ProcessSignalAction);

	memset(&m_datastruct, 0, sizeof(m_datastruct));

	Driver* driver;
	Connection* con;
	Statement* stmt;
	PreparedStatement* pstmt = NULL;
	ResultSet* result = NULL;


	try
	{
		driver = get_driver_instance();
		con = driver->connect(server, username, password);
	}

	catch (SQLException ex)
	{
		cout << "Could not Connect to server. Error Message " << ex.what() << endl;
		system("pause");
		exit(1);
	}

	con->setSchema("quickstartdb");
	stmt = con->createStatement();
	stmt->execute("DROP TABLE IF EXISTS inventory");
	cout << "Finished dropping table (if existed)" << endl;
	stmt->execute("CREATE TABLE inventory (seq INT NOT NULL AUTO_INCREMENT, id VARCHAR(100), pw VARCHAR(100), PRIMARY KEY(seq));");
	cout << "Finished creating table" << endl;
	delete stmt;


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
		listenSocket.Bind(Endpoint("0.0.0.0", 6000));

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
						}
						else
						{
							// 이미 수신된 상태이다. 수신 완료된 것을 그냥 꺼내 쓰자.
							unsigned char* echoData = remoteClient->tcpConnection.m_receiveBuffer;
							int echoDataLength = ec;

							// 원칙대로라면 TCP 스트림 특성상 일부만 송신하고 리턴하는 경우도 고려해야 하나,
							// 지금은 독자의 이해가 우선이므로, 생략하도록 한다.
							// 원칙대로라면 여기서 overlapped 송신을 해야 하지만 
							// 독자의 이해를 위해서 그냥 블로킹 송신을 한다.
							//0이면 성공, 1이면 잘못된 로그인 정보, 2면 중복된 회원가입 

							id_data m_data;
							sign_status m_signstatus;



							memcpy(&m_data, remoteClient->tcpConnection.m_receiveBuffer, sizeof(m_data));

							//remoteClient->tcpConnection.Send(echoData, echoDataLength);

							if (m_data.m_type == 0) //login
							{

								try
								{
									pstmt = con->prepareStatement("SELECT pw FROM inventory where id=(?)");
									pstmt->setString(1, reinterpret_cast<char*>(m_data.m_id));
									result = pstmt->executeQuery();

									if (!result->next())
									{
										cout << "login info incorrect" << endl;

										m_signstatus.m_val = -1;

										memcpy(remoteClient->tcpConnection.m_sendBuffer, &m_signstatus, sizeof(m_signstatus));

										if (remoteClient->tcpConnection.SendOverlapped(sizeof(m_signstatus)) != 0)
										{
											cout << "sending error" << endl;
										}



										//remoteClient->tcpConnection.Send(send_buffer, sizeof(send_buffer));

									}

									else
									{
										string query_pw = result->getString("pw");
										string cmp_pw = string(reinterpret_cast<char*>(m_data.m_pwd));

										cout << query_pw << " " << cmp_pw << endl;

										if (cmp_pw.compare(query_pw) == 0)
										{
											cout << "login success" << endl;

											m_signstatus.m_val = 0;

											memcpy(remoteClient->tcpConnection.m_sendBuffer, &m_signstatus, sizeof(m_signstatus));

											if (remoteClient->tcpConnection.SendOverlapped(sizeof(m_signstatus)) != 0)
											{
												cout << "sending error" << endl;
											}



											//	remoteClient->tcpConnection.Send(send_buffer, sizeof(send_buffer));


										}

										else
										{
											cout << "login info incorrect" << endl;

											m_signstatus.m_val = -1;

											memcpy(remoteClient->tcpConnection.m_sendBuffer, &m_signstatus, sizeof(m_signstatus));

											if (remoteClient->tcpConnection.SendOverlapped(sizeof(m_signstatus)) != 0)
											{
												cout << "sending error" << endl;
											}



											//	remoteClient->tcpConnection.Send(send_buffer, sizeof(send_buffer));

										}


									}


								}

								catch (SQLException ex)
								{
									cout << "Could not Sign . Error Message " << ex.what() << endl;
									system("pause");
									exit(1);

								}


							}

							else if (m_data.m_type == 1) // 회원가입
							{

								try
								{
									pstmt = con->prepareStatement("SELECT * FROM inventory where id=(?)");
									pstmt->setString(1, reinterpret_cast<char*>(m_data.m_id));
									result = pstmt->executeQuery();

									if (result->next())
									{
										cout << "There is id duplicated!" << endl;

										m_signstatus.m_val = -2;

										memcpy(remoteClient->tcpConnection.m_sendBuffer, &m_signstatus, sizeof(m_signstatus));

										if (remoteClient->tcpConnection.SendOverlapped(sizeof(m_signstatus)) != 0)
										{
											cout << "sending error" << endl;
										}


										//remoteClient->tcpConnection.Send(send_buffer, sizeof(send_buffer));
									}

									else
									{

										pstmt = con->prepareStatement("INSERT INTO inventory(id,pw) VALUES(?,?)");
										pstmt->setString(1, reinterpret_cast<char*>(m_data.m_id));
										pstmt->setString(2, reinterpret_cast<char*>(m_data.m_pwd));
										pstmt->execute();

										m_signstatus.m_val = 1;

										memcpy(remoteClient->tcpConnection.m_sendBuffer, &m_signstatus, sizeof(m_signstatus));

										if (remoteClient->tcpConnection.SendOverlapped(sizeof(m_signstatus)) != 0)
										{
											cout << "sending error" << endl;
										}

										//	remoteClient->tcpConnection.Send(send_buffer, sizeof(send_buffer));
									}

								}

								catch (SQLException ex)
								{
									cout << "Could not Sign . Error Message " << ex.what() << endl;
									system("pause");
									exit(1);

								}


							}



							// 다시 수신을 받으려면 overlapped I/O를 걸어야 한다.
							if (remoteClient->tcpConnection.ReceiveOverlapped() != 0
								&& WSAGetLastError() != ERROR_IO_PENDING)
							{
								ProcessClientLeave(remoteClient);
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




			}
		}

		// 사용자가 CTL-C를 눌러서 루프를 나갔다. 모든 종료를 하자.
		// 그러나 overlapped I/O가 모두 완료되기 전에는 그냥 무작정 나가면 안된다. 운영체제가 백그라운드로 overlapped I/O가 진행중이기 때문이다.
		// 완료를 모두 체크하고 나가도록 하자.
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

	delete result;
	delete pstmt;
	delete con;



	return 0;
}

