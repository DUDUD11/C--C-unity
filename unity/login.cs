using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;
using UnityEngine.SceneManagement;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System;
using UnityEngine.UI;


[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
[Serializable]
public struct AcceptPacket
{

    public int m_val;
    
}

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
[Serializable]
public struct SendClientID
{
    public int m_type;

    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 20)]
    public string m_id;

    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 20)]
    public string m_pwd;

    public SendClientID(int type, string id, string pwd)
    {
        m_type = type;
        m_id = id;
        m_pwd = pwd;
    }

}


public class Button : MonoBehaviour
{
    private Socket m_Client;
    public string m_Ip = "127.0.0.1";
    public int m_Port = 6000;
    private IPEndPoint m_ServerIpEndPoint;
    private EndPoint m_RemoteEndPoint;
    public static String m_Myid;
    public static int m_MyIdNumber=0;
    public static int m_WholeIdNumber=0;
    public TMP_InputField m_IdInputField;
    public TMP_InputField m_PwdInputField;
    public TMP_Text m_Status;

    private AcceptPacket m_AcceptPacket;
    private SendClientID m_sendClientID;

    public void Login()
    {
        Send(0);

    }

    public void Sign()
    {
        Send(1);
    }


    public void GotoGame()
    {
        SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex + 1);
    }


    // Start is called before the first frame update
    void Start()
    {
        m_MyIdNumber = 0;
        InitClient();

    }

    // Update is called once per frame
    void Update()
    {
        Receive();
    }

    void OnApplicationQuit()
    {
        CloseClient();
    }

    void InitClient()
    {

        m_ServerIpEndPoint = new IPEndPoint(IPAddress.Parse(m_Ip), m_Port);
        m_Client = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

        m_Client.Connect(m_ServerIpEndPoint);
    }

    public string toa(byte[] bytes)
    {
        return string.Join(", ", bytes);
    }


    public void Send(int x)
    {
        try
        {
        
            m_sendClientID = new SendClientID(x, m_IdInputField.text, m_PwdInputField.text);

            byte[] sendPacket = StructToByteArray(m_sendClientID);

            SendClientID sx = ByteArrayToStruct<SendClientID>(sendPacket);

            m_Client.Send(sendPacket, 0, sendPacket.Length, SocketFlags.None);
          
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


    void Receive()
    {

        int receive = 0;
        if (m_Client.Available != 0)
        {
            byte[] packet = new byte[32];

            try
            {
                receive = m_Client.Receive(packet);

            }

            catch (Exception ex)
            {
                Debug.Log(ex.ToString());
                return;
            }

            

            if (receive > 0)
            {

      
                m_AcceptPacket = ByteArrayToStruct<AcceptPacket>(packet);
                DoReceivePacket(m_AcceptPacket); // 받은 값 처리
            }
        }
    }

    void DoReceivePacket(AcceptPacket val)
    {


        //  m_Myid = val.m_val.ToString();

        if (val.m_val == -1)
        {
            m_Status.text = "wrong login information.";
        }

        else if (val.m_val == -2)
        {
            m_Status.text = "already registered account.";
        }

        else if (val.m_val == 1)
        {
            m_Status.text = "sign success";
            
        }

        else if (val.m_val == 0)
        {
            m_Status.text = "login success";
            m_Myid = m_IdInputField.text;

            GotoGame();
        }
    }

    void CloseClient()
    {
        if (m_Client != null)
        {
            m_Client.Close();
            m_Client = null;
        }
    }


    
    T ByteArrayToStruct<T>(byte[] buffer) where T : struct
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
    
}




