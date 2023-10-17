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


    private Socket m_Client;
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
    private ReceivePacket ReceiveClientID;
    private ReceivelobbyPlayer ReceivelobbyID;
    private SendClientID sendClientID;


    // Start is called before the first frame update


    private void Awake()
    {

        if (GameLobbyManager.gamelobbymanager == null)
        {
            GameLobbyManager.gamelobbymanager= this;
        }

        GL_Id = Button.m_Myid;
        InitClient();

        DontDestroyOnLoad(gameObject);



    }



    void Start()
    {
        GameReadyText1 = Panels[0].GetComponentsInChildren<TMP_Text>();
        GameReadyText2 = Panels[1].GetComponentsInChildren<TMP_Text>();
        GameReadyText3 = Panels[2].GetComponentsInChildren<TMP_Text>();
        GameReadyText4 = Panels[3].GetComponentsInChildren<TMP_Text>();
       


        Send_init();
    }




    // Update is called once per frame
    void Update()
    {

    }

    private void FixedUpdate()
    {
    //    Send();
        Receive();
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
        int len = gameObject.name.Length-6;

        int name = Int32.Parse(gameObject.name.Substring(6,len));

        try
        {

            sendClientID = new SendClientID(2,name, false, GL_Id);

            byte[] bytes = StructToByteArray(sendClientID);
            m_Client.Send(bytes, 0, bytes.Length, SocketFlags.None);

        }

        catch(Exception ex)
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


        int receive = 0;

        if (m_Client.Available != 0)
        {

            byte[] packet = new byte[m_ByteSize];
    

            try
            {
                receive = m_Client.Receive(packet);

                int tp = BitConverter.ToInt32(packet, 0); // 확인 필요

                if (tp > 1000000) return;

                Debug.Log(tp);

                if (tp == 5)
                {
                    GameManager.gamemanager.Get_Shell(packet);
                
                }

                else if (tp!=4)
                {
                    ReceiveClientID = ByteArrayToStruct<ReceivePacket>(packet);


                    if (receive > 0 && tp == 1)
                    {

                        UI_ID_Setting(ReceiveClientID);
                        GL_TankId = ReceiveClientID.m_TankId;

                    }

                    else if (receive > 0 && tp == 2)
                    {
                        // 이미 정원이 찼습니다.
                        UI_ID_Setting(ReceiveClientID);

                    }


                    else if (receive > 0 && tp == 3)
                    {

                        GL_Id = ReceiveClientID.m_id;
                        Play_Game();

                    }

                    else if (receive > 0 && tp == 50)
                    {
                        GL_Id = ReceiveClientID.m_id;
                        Play_Game();
                        //도중입장
                    }

                }

                else
                {
                    ReceivelobbyID = ByteArrayToStruct<ReceivelobbyPlayer>(packet);

                    GameLobbyText.text = ReceivelobbyID.m_players;

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



    private void UI_ID_Setting(ReceivePacket other_user)
    {
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


    private void Play_Game()
    {
            SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex + 1);
    }


    private bool Can_Enter(ReceivePacket client)
    {
        return client.m_Playing;
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
