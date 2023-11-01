using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;
using UnityEngine.UI;

using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using UnityEngine.SceneManagement;
using System.Threading;



public class GameLobbyManager : MonoBehaviour
{

    public static GameLobbyManager gamelobbymanager;


    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    [Serializable]
    public struct SendClientID
    {

        public int m_Type;

        public int m_TankId;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 20)]
        public string m_id;

        [MarshalAs(UnmanagedType.U1)]
        public bool m_Playing;

        public SendClientID(int type, int tankid, bool playing, string id)
        {
            m_Type = type;
            m_TankId = tankid;
            m_Playing = playing;
            m_id = id;
        }

    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    [Serializable]
    public struct ReceivePacket
    {
        [MarshalAs(UnmanagedType.I4)]
        public int m_Type;

        [MarshalAs(UnmanagedType.I4)]
        public int m_TankId;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 20)]
        public string m_id;

        [MarshalAs(UnmanagedType.U1)]
        public bool m_Playing;

        public ReceivePacket(int type, int tankid, bool playing, string id)
        {
            m_Type = type;
            m_TankId = tankid;
            m_Playing = playing;
            m_id = id;
        }

    }

    public struct ReceivelobbyPlayer
    {
        [MarshalAs(UnmanagedType.I4)]
        public int m_type;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 500)]
        public string m_players;

        public ReceivelobbyPlayer(int type, string players)
        {
            m_type = type;
            m_players = players;
        }

    }

    public struct SendChat
    {
        [MarshalAs(UnmanagedType.I4)]
        public int m_type;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 20)]
        public string m_id;

        [MarshalAs(UnmanagedType.I4)]
        public int m_len;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 100)]
        public string m_chats;

        public SendChat(int type, string id, int len, string chats)
        {
            m_type = type;
            m_id = id;
            m_len = len;
            m_chats = chats;
        }

    }

    public struct ReceiveChat
    {
        [MarshalAs(UnmanagedType.I4)]
        public int m_type;

        [MarshalAs(UnmanagedType.I4)]
        public int m_len;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 500)]
        public string m_chats;

        public ReceiveChat(int type, string id, int len, string chats)
        {
            m_type = type;
            m_len = len;
            m_chats = chats;
        }


    }

    public struct ReceiveChat_Remain
    {

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 500)]
        public string m_chats;

        public ReceiveChat_Remain(string chats)
        {
            m_chats = chats;
        }


    }




    private Socket m_Client;
    private Thread m_ThirdClientReceive;
    public string m_Ip = "127.0.0.1";
    public int m_Port = 6200;
    private IPEndPoint m_ServerIpEndPoint;
    private EndPoint m_RemoteEndPoint;


    private int m_ByteSize = 600;

    public static String GL_Id;
    public static int GL_TankId;
    public GameObject[] Panels;
    public TMP_Text[] GameReadyText1;
    public TMP_Text[] GameReadyText2;
    public TMP_Text[] GameReadyText3;
    public TMP_Text[] GameReadyText4;
    public TMP_Text GameLobbyText;


    public TMP_InputField GameLobbyChatInput;
    public TMP_Text GameLobbyChat;


    private ReceivePacket ReceiveClientID;
    private ReceivelobbyPlayer ReceivelobbyID;
    private SendClientID sendClientID;
    private ReceiveChat receiveChat;
    private ReceiveChat_Remain receiveChat_Remain;

    private Queue<ReceivePacket> UI_Queue;
    private Boolean lobby_update;
    private Boolean next_scene;

    // 0은 처리 끝이거나 할게없거나, 1은 처리가 필요할때, 2는 남은 챗이 있을떄
    private int Buffer_end;

    private String Buffer;



    // Start is called before the first frame update


    private void Awake()
    {

        if (GameLobbyManager.gamelobbymanager == null)
        {
            GameLobbyManager.gamelobbymanager = this;
        }

        GL_Id = Button.m_Myid;
        InitClient();

        DontDestroyOnLoad(gameObject);



    }



    void Start()
    {
        UI_Queue = new Queue<ReceivePacket>();
        lobby_update = false;
        next_scene = false;

        GameReadyText1 = Panels[0].GetComponentsInChildren<TMP_Text>();
        GameReadyText2 = Panels[1].GetComponentsInChildren<TMP_Text>();
        GameReadyText3 = Panels[2].GetComponentsInChildren<TMP_Text>();
        GameReadyText4 = Panels[3].GetComponentsInChildren<TMP_Text>();

        Send_init();

        Thread_init();

        Buffer_end = 0;
    }

    private void Thread_init()
    {
        try
        {
            m_ThirdClientReceive = new Thread(new ThreadStart(Receive));
            m_ThirdClientReceive.IsBackground = true;
            m_ThirdClientReceive.Start();

        }

        catch (Exception ex)
        {
            Debug.Log(ex);
        }




    }

    public void Chat_Send()
    {

        try
        {

            SendChat Chat = new SendChat(10, GL_Id, GameLobbyChatInput.text.Length, GameLobbyChatInput.text);
            byte[] bytes = StructToByteArray(Chat);
            m_Client.Send(bytes, 0, bytes.Length, SocketFlags.None);

        }

        catch (Exception ex)
        {
            Debug.Log(ex.ToString());
            return;
        }
    }







    private void OnApplicationQuit()
    {
        m_ThirdClientReceive.Abort();

        if (m_Client != null)
        {
            m_Client.Close();
            m_Client = null;
        }
    }



    // Update is called once per frame
    void Update()
    {
        if (lobby_update)
        {
            GameLobbyText.text = ReceivelobbyID.m_players;
            Debug.Log(ReceivelobbyID.m_players);
            Debug.Log(GameLobbyText.text);

            lobby_update = false;
        }

        if (Buffer_end == 1)
        {
            GameLobbyChat.text = Buffer;
            Buffer = "";
            Buffer_end = 0;
        }

    }

    private void FixedUpdate()
    {
        //    Send();
        if (UI_Queue.Count != 0)
        {
            UI_ID_Setting(UI_Queue);
        }

        if (next_scene)
        {
            Play_Game();
            next_scene = false;
        }
    }

    private void InitClient()
    {
        m_ServerIpEndPoint = new IPEndPoint(IPAddress.Parse(m_Ip), m_Port);
        m_Client = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

        m_Client.Connect(m_ServerIpEndPoint);

    }

    public IPEndPoint GETIPENDPOINT()
    {
        return m_ServerIpEndPoint;
    }

    public Socket GETSOCKET()
    {
        return m_Client;
    }




    private void Send_init()
    {

        try
        {
            SendClientID tempID = new SendClientID(3, 0, false, GL_Id);

            byte[] bytes = StructToByteArray(tempID);
            m_Client.Send(bytes, 0, bytes.Length, SocketFlags.None);


            //            SetSendPacket();
            //           byte[] bytes = StructToByteArray(m_SendPacket);
            //         m_Client.Send(bytes, 0, bytes.Length, SocketFlags.None);
            /*
           Debug.Log($"[Send] {m_Ip}:{m_Port} Size : {bytes.Length} byte");



           Debug.LogFormat($"{m_SendPacket.m_Transform[0]} " +
           $"{m_SendPacket.m_Transform[1]} " +
           $"{m_SendPacket.m_Transform[2]} " +
           $"m_SendPacket.m_TankId = {m_SendPacket.m_TankId } " +
           $"m_SendPacket.m_Health = { m_SendPacket.m_Health} " 
           );

           */

        }

        catch (Exception ex)
        {
            Debug.Log(ex.ToString());
            return;

        }




    }





    // setsendpacket receive

    public void SendClicked(GameObject gameObject)
    {
        int len = gameObject.name.Length - 6;

        int name = Int32.Parse(gameObject.name.Substring(6, len));

        try
        {

            sendClientID = new SendClientID(2, name, false, GL_Id);

            byte[] bytes = StructToByteArray(sendClientID);
            m_Client.Send(bytes, 0, bytes.Length, SocketFlags.None);



        }

        catch (Exception ex)
        {
            Debug.Log(ex.ToString());
            return;
        }


    }

    byte[] StructToByteArray(object obj)
    {
        int size = Marshal.SizeOf(obj);
        byte[] arr = new byte[size];
        IntPtr ptr = Marshal.AllocHGlobal(size);

        Marshal.StructureToPtr(obj, ptr, true);
        Marshal.Copy(ptr, arr, 0, size);
        Marshal.FreeHGlobal(ptr);
        return arr;
    }

    private void Receive()
    {

        while (true)
        {

            int receive = 0;

            byte[] packet = new byte[m_ByteSize];




            try
            {
                receive = m_Client.Receive(packet);

                int tp = BitConverter.ToInt32(packet, 0); // 확인 필요

                Debug.Log(tp + " " + receive + " " + Marshal.SizeOf(typeof(ReceiveChat)));



                if (receive == Marshal.SizeOf(typeof(ReceiveChat)))
                {
                    tp = 10;

                }

                if (tp == 10)

                {
                    if (Buffer_end != 2)
                    {
                        receiveChat = ByteArrayToStruct<ReceiveChat>(packet);

                        if (receiveChat.m_len > 500)
                        {
                            Buffer_end = 2;
                            Buffer += receiveChat.m_chats;

                        }

                        else
                        {
                            Buffer_end = 1;
                            Buffer += receiveChat.m_chats;

                        }


                    }


                    else
                    {
                        receiveChat_Remain = ByteArrayToStruct<ReceiveChat_Remain>(packet);

                        if (receiveChat_Remain.m_chats[500] == '\0')
                        {
                            Buffer_end = 1;
                        }

                        Buffer += receiveChat_Remain.m_chats;


                    }

                }



                else if (tp == 5)
                {


                    GameManager.gamemanager.Get_Shell(packet);

                }

                else if (tp != 4)
                {
                    ReceiveClientID = ByteArrayToStruct<ReceivePacket>(packet);


                    if (receive > 0 && tp == 1)
                    {
                        UI_Queue.Enqueue(ReceiveClientID);

                        GL_TankId = ReceiveClientID.m_TankId;

                    }

                    else if (receive > 0 && tp == 2)
                    {
                        // 이미 정원이 찼습니다.
                        UI_Queue.Enqueue(ReceiveClientID);

                    }


                    else if (receive > 0 && tp == 3)
                    {

                        GL_Id = ReceiveClientID.m_id;
                        next_scene = true;

                    }

                    else if (receive > 0 && tp == 50)
                    {
                        GL_Id = ReceiveClientID.m_id;
                        GL_TankId = ReceiveClientID.m_TankId;
                        next_scene = true;
                        //도중입장
                    }

                }

                else
                {
                    ReceivelobbyID = ByteArrayToStruct<ReceivelobbyPlayer>(packet);

                    lobby_update = true;


                }

                /*

                Debug.LogFormat($"{ReceiveClientID.m_id} " +
                $"{ReceiveClientID.m_Playing} " +
                $"{ReceiveClientID.m_TankId} " +
                $"{ReceiveClientID.m_Type} "
                );

                */


            }

            catch (Exception ex)
            {
                Debug.Log(ex.ToString());
                return;
            }








        }

    }



    private void UI_ID_Setting(Queue<ReceivePacket> other_users)
    {



        while (other_users.Count != 0)
        {

            ReceivePacket other_user = other_users.Dequeue();


            int x = other_user.m_TankId;



            for (int i = 0; i < 4; i++)
            {

                int len = GameReadyText1[i].name.Length - 10; // TextPlayer

                int name = Int32.Parse(GameReadyText1[i].name.Substring(10, len));

                if (name == x)
                {
                    GameReadyText1[i].text = other_user.m_id;
                }

                len = GameReadyText2[i].name.Length - 10;

                name = Int32.Parse(GameReadyText2[i].name.Substring(10, len));

                if (name == x)
                {
                    GameReadyText2[i].text = other_user.m_id;
                }

                len = GameReadyText3[i].name.Length - 10;

                name = Int32.Parse(GameReadyText3[i].name.Substring(10, len));

                if (name == x)
                {
                    GameReadyText3[i].text = other_user.m_id;
                }

                len = GameReadyText4[i].name.Length - 10;

                name = Int32.Parse(GameReadyText4[i].name.Substring(10, len));

                if (name == x)
                {
                    GameReadyText4[i].text = other_user.m_id;
                }





            }

        }

    }


    private void Play_Game()
    {
        SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex + 1);
    }

    private T ByteArrayToStruct<T>(byte[] buffer) where T : struct
    {
        int size = Marshal.SizeOf(typeof(T));


        // Debug.Log(string.Join(", ", buffer));


        if (size > buffer.Length)
        {
            throw new Exception();
        }

        IntPtr ptr = Marshal.AllocHGlobal(size);
        Marshal.Copy(buffer, 0, ptr, size);
        T obj = (T)Marshal.PtrToStructure(ptr, typeof(T));
        Marshal.FreeHGlobal(ptr);
        return obj;
    }
}
