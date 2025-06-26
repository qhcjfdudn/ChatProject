using System;
using System.Net.Sockets;
using System.Text;
using UnityEngine;

using static NetworkUtils;

public enum PacketType
{
    PT_Hello,
    PT_ReplicationData,
    PT_Disconnect,
    PT_Max
};

public class NetworkManager : MonoBehaviour
{
    private static NetworkManager _instance;
    // private NetworkManagerUI _networkManagerUI;

    public static NetworkManager Instance
    {
        get
        {
            if (_instance == null)
            {
                Debug.Log("NetworkManager GameObject�� ���� �������� �ʾҽ��ϴ�. instance�� null�� return�մϴ�.");
            }
            return _instance;
        }
    }

    private TcpClient _client;
    private NetworkStream _stream;
    private byte[] _receiveBuffer = new byte[1024];
    private OutputMemoryBitStream _outBuffer = new OutputMemoryBitStream();
    public OutputMemoryBitStream OutBuffer { get { return _outBuffer; } }

    public string _serverIP = "127.0.0.1"; // ���� IP �ּ�
    public int _serverPort = 50000;       // ���� ��Ʈ ��ȣ

    private ReplicationManager _replicationManager;

    private NetworkManager() { }

    void Awake()
    {
        if (_instance != null && _instance != this)
        {
            Destroy(gameObject);
            return;
        }

        _instance = this;
        DontDestroyOnLoad(gameObject);

        _replicationManager = ReplicationManager.Instance;
    }

    private void Start()
    {
        //_networkManagerUI = NetworkManagerUI.Instance;
        //_networkManagerUI.gameObject.transform.SetParent(transform);

        ConnectToServer();
    }

    // ���� ����
    private void OnApplicationQuit()
    {
        if (_client != null)
        {
            _stream?.Close();
            _client?.Close();
        }
    }

    // ���� ����
    public void ConnectToServer()
    {
        try
        {
            _client = new TcpClient();
            _client.Connect(_serverIP, _serverPort); // ������ ����
            _stream = _client.GetStream();

            Debug.Log("������ ����Ǿ����ϴ�.");

            StartListening(); // ������ ���� ���
        }
        catch (Exception ex)
        {
            Debug.LogError("���� ���� ����: " + ex.Message);
        }
    }

    // ������ ���� ���
    private async void StartListening()
    {
        try
        {
            while (_client != null && _client.Connected)
            {
                int bytesRead = await _stream.ReadAsync(_receiveBuffer, 0, _receiveBuffer.Length);
                // bytesRead�� �ֱ� ������, ������� �о� ���� bytes�� ���� ������ �����ͷ� 0�� ���� �ʾƵ� �ȴ�.
                if (bytesRead > 0)
                {
                    Debug.Log($"bytesRead: {bytesRead}");
                    InputMemoryBitStream outStream = new InputMemoryBitStream(_receiveBuffer, bytesRead);
                    Debug.Log($"_receiveBuffer.ToString(): {BitConverter.ToString(outStream.StreamBuffer)}");

                    PacketType pt = (PacketType)outStream.ReadBits(GetRequiredBits((int)PacketType.PT_Max))[0];
                    Debug.Log($"pt: {pt.ToString()}");

                    switch (pt)
                    {
                        case PacketType.PT_Hello:
                            break;

                        case PacketType.PT_ReplicationData:
                            _replicationManager.ProcessReplicationAction(outStream);
                            break;

                        case PacketType.PT_Disconnect:
                            break;

                        default: break;
                    }


                    //ProcessIncomingDataTest(outStream);
                }
            }
        }
        catch (Exception ex)
        {
            Debug.LogError("�������� ������ ����Ǿ����ϴ�: " + ex.Message);
            Debug.Log(ex.StackTrace.ToString());
        }
    }

    private void ProcessIncomingDataTest(InputMemoryBitStream inputBitStream)
    {
        while (true)
        {
            int packetHeader = inputBitStream.ReadBits(2)[0];
            if (packetHeader == 0)
            {
                break;
            }

            if (packetHeader == 1)
            {
                UnwrapAddNewChat(inputBitStream);
            }
            else if (packetHeader == 2)
            {
                UnwrapSetUsername(inputBitStream);
            }
        }
    }

    public void SendPacketsToServer()
    {
        if (null == _client || false == _client.Connected)
        {
            PrintNotConnectedToServerMessage();
            return;
        }

        try
        {
            Debug.Log($"before _outBuffer.ToString(): {BitConverter.ToString(_outBuffer.StreamBuffer)}");
            _outBuffer.WriteBits(0, 2);
            Debug.Log($"after _outBuffer.ToString(): {BitConverter.ToString(_outBuffer.StreamBuffer)}");
            _stream.Write(_outBuffer.StreamBuffer, 0, _outBuffer.Count); // �޽��� ����
            _outBuffer.InitBuffer();
        }
        catch (Exception ex)
        {
            Debug.LogError("�޽��� ���� ����: " + ex.Message);
        }
    }

    private void PrintNotConnectedToServerMessage()
    {
        Debug.LogError("������ ������� �ʾҽ��ϴ�.");
    }

    private void UnwrapAddNewChat(InputMemoryBitStream inputBitStream)
    {
        int stringLength = inputBitStream.ReadBits(8)[0];
        Debug.Log($"stringLength: {stringLength}");
        string newChat = Encoding.UTF8.GetString(inputBitStream.ReadBits(stringLength * 8));
        Debug.Log($"newChat: {newChat}");

        FindAnyObjectByType<ChatController>().AddNewChat(newChat);
    }

    private void UnwrapSetUsername(InputMemoryBitStream inputBitStream)
    {
        int stringLength = inputBitStream.ReadBits(8)[0];
        Debug.Log($"stringLength: {stringLength}");
        string newUsername = Encoding.UTF8.GetString(inputBitStream.ReadBits(stringLength * 8));
        Debug.Log($"newUsername: {newUsername}");
        FindAnyObjectByType<UIController>().SetUsername(newUsername);
    }
}
