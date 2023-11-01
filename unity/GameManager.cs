


using UnityEngine;
using System.Collections;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

// network manager section
using System;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;


public class GameManager : MonoBehaviour
{
    public static GameManager gamemanager;

    //network udp
    private UdpClient m_Client;
    public string m_Ip = "127.0.0.1";
    public int m_Port = 5555;
    private PacketExchange m_SendPacket;
    private PacketExchange m_ReceivePacket;
    private IPEndPoint m_RemoteIpEndPoint;
    private int m_ByteSize = 32;
    private bool m_IsThreading;
    private Thread m_ThrdReceive;


    //network tcp
    private Socket m_Client_TCP;
    public string m_Ip_TCP = "127.0.0.1";
    public int m_Port_TCP = 6200;
    private IPEndPoint m_ServerIpEndPoint;
    private EndPoint m_RemoteEndPoint_TCP;
    public ShellSend m_SendshellFireInformation;
    public ShellFireInformation m_ReceiveshellFireInformation;
    // private int m_ByteSize_TCP = 44;

    public string m_Myid;
    public int m_TankId;
    public int m_TankUsers = 4;
    public int m_NumRoundsToWin = 5;
    public float m_StartDelay = 3f;
    public float m_EndDelay = 3f;
    public CameraControl m_CameraControl;
    public Text m_MessageText;
    public GameObject m_TankPrefab;
    public TankManager[] m_Tanks;


    private int m_RoundNumber = 0;
    private WaitForSeconds m_StartWait;
    private WaitForSeconds m_EndWait;
    private TankManager m_RoundWinner;
    private TankManager m_GameWinner;
    private TankHealth m_TankHealth;
    private TankShooting[] m_TankShooting;

    private Boolean enemyTargetStatus;
    public Boolean get_shell;
    private byte[] pack;


    private void Awake()
    {
        // singleton 
        if (GameManager.gamemanager == null)
        {
            GameManager.gamemanager = this;
        }

        InitClient();
        m_Myid = GameLobbyManager.GL_Id;
        m_TankId = GameLobbyManager.GL_TankId;
        // m_TankUsers = Button.m_WholeIdNumber;
    }

    private void Start()
    {

        m_TankShooting = new TankShooting[m_TankUsers];


        m_StartWait = new WaitForSeconds(m_StartDelay);
        m_EndWait = new WaitForSeconds(m_EndDelay);


        SpawnAllTanks();
        SetCameraTargets();

        StartCoroutine(GameLoop());

        enemyTargetStatus = false;

        Debug.Log(m_TankId);

        m_TankHealth = m_Tanks[m_TankId % 4].m_Instance.GetComponent<TankHealth>();
        for (int i = 0; i < m_TankUsers; i++)
        {
            m_TankShooting[i] = m_Tanks[i].m_Instance.GetComponent<TankShooting>();
        }

        get_shell = false;
        // Send();

    }



    private void FixedUpdate()
    {
        Application.targetFrameRate = 1 / 60;

        SetSendPacket();
        if (enemyTargetStatus)
        {
            SetEnemyTankStatus(m_ReceivePacket);

            enemyTargetStatus = false;
        }

        if (get_shell)
        {
            Shell_do();
            get_shell = false;

        }

        //  Send();
        //  Receive();   
    }

    private void InitClient()
    {
        //udp


        m_RemoteIpEndPoint = new IPEndPoint(IPAddress.Parse(m_Ip), m_Port);
        m_Client = new UdpClient();


        m_ThrdReceive = new Thread(Send);
        m_IsThreading = true;

        if (m_ThrdReceive.ThreadState != ThreadState.Running)
        {
            m_ThrdReceive.Start();
        }

        //   m_Client.Client.Blocking = false;




        m_SendPacket.m_Transform = new float[3];
        m_SendPacket.m_Rotation = new float[3];


        //tcp

        // m_ServerIpEndPoint = new IPEndPoint(IPAddress.Parse(m_Ip_TCP), m_Port_TCP);

        m_ServerIpEndPoint = GameLobbyManager.gamelobbymanager.GETIPENDPOINT();


        // m_Client_TCP = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        m_Client_TCP = GameLobbyManager.gamelobbymanager.GETSOCKET();


        //  m_Client_TCP.Connect(m_ServerIpEndPoint);

        m_SendshellFireInformation.type = 5;

        m_SendshellFireInformation.m_Transform = new float[3];
        m_SendshellFireInformation.m_Rotation = new float[4];
        m_SendshellFireInformation.m_Velocity = new float[3];


    }

    private void Send()
    {

        while (m_IsThreading)
        {

            Thread.Sleep(50);


            try
            {




                byte[] bytes = StructToByteArray(m_SendPacket);
                m_Client.Send(bytes, bytes.Length, m_Ip, m_Port);

                /*
               Debug.Log($"[Send] {m_Ip}:{m_Port} Size : {bytes.Length} byte");



               Debug.LogFormat($"{m_SendPacket.m_Transform[0]} " +
               $"{m_SendPacket.m_Transform[1]} " +
               $"{m_SendPacket.m_Transform[2]} " +
               $"m_SendPacket.m_TankId = {m_SendPacket.m_TankId } " +
               $"m_SendPacket.m_Health = { m_SendPacket.m_Health} " 
               );

               */


                Receive();

            }

            catch (Exception ex)
            {
                Debug.Log(ex.ToString());
                return;

            }
        }


    }

    public void Fire_Transmission(Vector3 pos, Quaternion rot, Vector3 vel)
    {
        try

        {

            m_SendshellFireInformation.m_Transform[0] = pos.x;
            m_SendshellFireInformation.m_Transform[1] = pos.y;
            m_SendshellFireInformation.m_Transform[2] = pos.z;

            m_SendshellFireInformation.m_Rotation[0] = rot.x;
            m_SendshellFireInformation.m_Rotation[1] = rot.y;
            m_SendshellFireInformation.m_Rotation[2] = rot.z;
            m_SendshellFireInformation.m_Rotation[3] = rot.w;

            m_SendshellFireInformation.m_Velocity[0] = vel.x;
            m_SendshellFireInformation.m_Velocity[1] = vel.y;
            m_SendshellFireInformation.m_Velocity[2] = vel.z;

            m_SendshellFireInformation.m_TankId = m_TankId;


            byte[] sendPacket = StructToByteArray(m_SendshellFireInformation);
            m_Client_TCP.Send(sendPacket, 0, sendPacket.Length, SocketFlags.None);



        }

        catch (Exception ex)
        {
            Debug.Log(ex.ToString());
            return;
        }



    }

    public void Shell_do()
    {

        try
        {

            m_ReceiveshellFireInformation = ByteArrayToStruct<ShellFireInformation>(pack);


            /*
            Debug.Log($"[Receive] {packet.Length} byte");


    Debug.LogFormat($"{m_ReceiveshellFireInformation.m_Transform[0]} " +
    $"{m_ReceiveshellFireInformation.m_Transform[1]} " +
    $"{m_ReceiveshellFireInformation.m_Transform[2]} " +
    $"{m_ReceiveshellFireInformation.m_Rotation[0]} " +
    $"{m_ReceiveshellFireInformation.m_Rotation[1]} " +
    $"{m_ReceiveshellFireInformation.m_Rotation[2]} " +
    $"{m_ReceiveshellFireInformation.m_Rotation[3]} " +
    $"m_ReceivePacket.m_Health = { m_ReceivePacket.m_Health} "
    );
              */


        }

        catch (Exception ex)
        {
            Debug.Log(ex.ToString());
            return;
        }


        Generate_Enemy_Shell(m_ReceiveshellFireInformation); // 받은 값 처리

    }


    public void Get_Shell(byte[] packet)
    {
        get_shell = true;
        pack = packet;
    }

    private void Generate_Enemy_Shell(ShellFireInformation recv)
    {
        int i = recv.m_TankId % 4;

        Vector3 pos = new Vector3(recv.m_Transform[0], recv.m_Transform[1], recv.m_Transform[2]);
        Quaternion rot = new Quaternion(recv.m_Rotation[0], recv.m_Rotation[1], recv.m_Rotation[2], recv.m_Rotation[3]);
        Vector3 vel = new Vector3(recv.m_Velocity[0], recv.m_Velocity[1], recv.m_Velocity[2]);

        Rigidbody ShellInstance = Instantiate(m_TankShooting[i].m_Shell, pos, rot) as Rigidbody;

        ShellInstance.velocity = vel;


    }

    private void Receive()
    {


        if (m_Client.Available != 0)
        {


            byte[] packet = new byte[m_ByteSize];

            try
            {
                packet = m_Client.Receive(ref m_RemoteIpEndPoint);

                m_ReceivePacket = ByteArrayToStruct<PacketExchange>(packet);


                /*

                  Debug.Log($"[Receive] {m_RemoteIpEndPoint.Address}:{m_RemoteIpEndPoint.Port} Size : {packet.Length} byte");


                 Debug.LogFormat($"{m_ReceivePacket.m_Transform[0]} " +
                 $"{m_ReceivePacket.m_Transform[1]} " +
                 $"{m_ReceivePacket.m_Transform[2]} " +
                 $"{m_ReceivePacket.m_Rotation[0]} " +
                 $"{m_ReceivePacket.m_Rotation[1]} " +
                 $"{m_ReceivePacket.m_Rotation[2]} " +

                 $"m_ReceivePacket.m_TankId = {m_ReceivePacket.m_TankId } " +
                 $"m_ReceivePacket.m_Health = { m_ReceivePacket.m_Health} "
                 );

                 */
                enemyTargetStatus = true;
            }

            catch (Exception ex)
            {
                Debug.Log(ex.ToString());
            }



        }



        //DoReceivePacket();

    }

    private void SetEnemyTankStatus(PacketExchange packetenemy)
    {
        int r_id = packetenemy.m_TankId;

        // 수정사항
        r_id %= 4;

        m_Tanks[r_id].m_Instance.GetComponent<TankMovement>().Enemy_Move(new Vector3(packetenemy.m_Transform[0], packetenemy.m_Transform[1], packetenemy.m_Transform[2]));
        m_Tanks[r_id].m_Instance.transform.localEulerAngles = new Vector3(0f, packetenemy.m_Rotation[1], 0f);
        // m_Tanks[r_id].m_Instance.transform.localEulerAngles = Quaternion.Slerp(m_Tanks[r_id].m_Instance.transform.rotation, packetenemy.m_Rotation, 2); 
        //test[r_id]++;

        //Debug.LogFormat($" {test[0]} " + $" {test[1]}" + $" {test[2]} " + $" {test[3]}" );

        //m_TankHealth[r_id].SetCurrentHealth(packetenemy.m_Health); 따로 처리 필요

    }


    private void SetSendPacket()
    {

        m_SendPacket.m_Transform[0] = m_Tanks[m_TankId % 4].m_Instance.transform.position.x;
        m_SendPacket.m_Transform[1] = m_Tanks[m_TankId % 4].m_Instance.transform.position.y;
        m_SendPacket.m_Transform[2] = m_Tanks[m_TankId % 4].m_Instance.transform.position.z;

        m_SendPacket.m_Rotation[0] = m_Tanks[m_TankId % 4].m_Instance.transform.localEulerAngles.x;
        m_SendPacket.m_Rotation[1] = m_Tanks[m_TankId % 4].m_Instance.transform.localEulerAngles.y;
        m_SendPacket.m_Rotation[2] = m_Tanks[m_TankId % 4].m_Instance.transform.localEulerAngles.z;

        m_SendPacket.m_TankId = m_TankId;
        m_SendPacket.m_Health = m_TankHealth.GetCurrentHealth();
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

    private void OnApplicationQuit()
    {



        CloseClient();
    }

    private void CloseClient()
    {
        m_IsThreading = false;

        if (m_Client != null) m_Client.Close();


        if (m_ThrdReceive != null)
        {
            try
            {
                m_ThrdReceive.Abort();
            }

            catch (Exception e)
            {
                Debug.Log(e.ToString());
            }


        }



        m_Client_TCP.Close();
    }


    private T ByteArrayToStruct<T>(byte[] buffer) where T : struct
    {
        int size = Marshal.SizeOf(typeof(T));
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

    private void SpawnAllTanks()
    {



        for (int i = 0; i < m_TankUsers; i++)
        {
            m_Tanks[i].m_Instance =
                Instantiate(m_TankPrefab, m_Tanks[i].m_SpawnPoint.position, m_Tanks[i].m_SpawnPoint.rotation) as GameObject;

            if (i == m_TankId % 4)
            {
                m_Tanks[i].m_PlayerNumber = 1; // 자신의 번호로 바꿀필요가 있음
                m_Tanks[i].Set_name(m_TankId + "");
            }

            else
            {
                m_Tanks[i].m_PlayerNumber = 200;
                m_Tanks[i].Set_name((m_TankId + i) + "");
            }

            m_Tanks[i].Setup();
        }
    }


    private void SetCameraTargets()
    {
        Transform[] targets = new Transform[m_TankUsers];

        for (int i = 0; i < targets.Length; i++)
        {

            targets[i] = m_Tanks[i].m_Instance.transform;
            Debug.Log(i + " " + targets[i]);
        }

        m_CameraControl.m_Targets = targets;
    }


    private IEnumerator GameLoop()
    {
        yield return StartCoroutine(RoundStarting());
        yield return StartCoroutine(RoundPlaying());
        yield return StartCoroutine(RoundEnding());

        if (m_GameWinner != null)
        {
            SceneManager.LoadScene(0);
        }
        else
        {
            StartCoroutine(GameLoop());
        }
    }


    private IEnumerator RoundStarting()
    {
        ResetAllTanks();
        DisableTankControl();

        m_CameraControl.SetStartPositionAndSize();

        m_RoundNumber++;
        m_MessageText.text = "ROUND " + m_RoundNumber;

        yield return m_StartWait;
    }


    private IEnumerator RoundPlaying()
    {
        EnableTankControl();

        m_MessageText.text = string.Empty;

        while (!OneTankLeft())
        {
            yield return null;

        }

        yield return null;
    }


    private IEnumerator RoundEnding()
    {
        DisableTankControl();

        m_RoundWinner = null;

        m_RoundWinner = GetRoundWinner();

        if (m_RoundWinner != null)
            m_RoundWinner.m_Wins++;

        m_GameWinner = GetGameWinner();

        string message = EndMessage();
        m_MessageText.text = message;

        yield return m_EndWait;
    }


    private bool OneTankLeft()
    {
        int numTanksLeft = 0;

        for (int i = 0; i < m_TankUsers; i++)
        {
            if (m_Tanks[i].m_Instance.activeSelf)
                numTanksLeft++;
        }

        return numTanksLeft <= 1;
    }


    private TankManager GetRoundWinner()
    {
        for (int i = 0; i < m_TankUsers; i++)
        {
            if (m_Tanks[i].m_Instance.activeSelf)
                return m_Tanks[i];
        }

        return null;
    }


    private TankManager GetGameWinner()
    {
        for (int i = 0; i < m_TankUsers; i++)
        {
            if (m_Tanks[i].m_Wins == m_NumRoundsToWin)
                return m_Tanks[i];
        }

        return null;
    }


    private string EndMessage()
    {
        string message = "DRAW!";

        if (m_RoundWinner != null)
            message = m_RoundWinner.m_ColoredPlayerText + " WINS THE ROUND!";

        message += "\n\n\n\n";

        for (int i = 0; i < m_TankUsers; i++)
        {
            message += m_Tanks[i].m_ColoredPlayerText + ": " + m_Tanks[i].m_Wins + " WINS\n";
        }

        if (m_GameWinner != null)
            message = m_GameWinner.m_ColoredPlayerText + " WINS THE GAME!";

        return message;
    }


    private void ResetAllTanks()
    {
        for (int i = 0; i < m_TankUsers; i++)
        {
            m_Tanks[i].Reset();
        }
    }


    private void EnableTankControl()
    {
        for (int i = 0; i < m_TankUsers; i++)
        {
            m_Tanks[i].EnableControl();
        }
    }


    private void DisableTankControl()
    {
        for (int i = 0; i < m_TankUsers; i++)
        {
            m_Tanks[i].DisableControl();
        }
    }
}