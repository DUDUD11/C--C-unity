using System;
using System.Runtime.InteropServices;

// network struct
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
[Serializable]
public struct PacketExchange
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public float[] m_Transform;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public float[] m_Rotation;
    //[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
    //public string m_StringlVariable;
    //[MarshalAs(UnmanagedType.Bool)]
    //public bool m_BoolVariable;
    public int m_TankId;
    public float m_Health;
}


[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
[Serializable]

public struct ShellFireInformation
{

    public int type;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public float[] m_Transform;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public float[] m_Velocity;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
    public float[] m_Rotation;

    public int m_TankId;

}

public struct ShellSend
{
    public int type;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public float[] m_Transform;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public float[] m_Velocity;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
    public float[] m_Rotation;

    public int m_TankId;

}
