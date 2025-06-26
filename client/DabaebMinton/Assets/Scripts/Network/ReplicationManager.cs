using UnityEngine;

public enum ReplicationAction
{
    RA_Create,
    RA_Update,
    RA_Delete,
    RA_Max
};

public class ReplicationManager : MonoBehaviour
{
    public static ReplicationManager Instance
    {
        get
        {
            if (_instance == null)
            {
                Debug.Log("ReplicationManager GameObject�� ���� �������� �ʾҽ��ϴ�. instance�� null�� return�մϴ�.");
            }
            return _instance;
        }
    }
    private static ReplicationManager _instance;

    public void ProcessReplicationAction(InputMemoryBitStream inStream)
    {
        ReplicationHeader rh = new ReplicationHeader();
        rh.Read(inStream);

        Debug.Log($"rh.Ra: {rh.Ra}, rh.Nid: {rh.Nid}, rh.Cid: {rh.Cid}");

        switch (rh.Ra)
        {
            case ReplicationAction.RA_Create:
                ReplicationCreate();
                break;

            case ReplicationAction.RA_Update:
                ReplicationUpdate(rh, inStream);
                break;

            case ReplicationAction.RA_Delete:
                ReplicationDelete();
                break;
        }
    }

    public void ReplicationCreate()
    {
        Debug.Log("RA_Create ���� �Ϸ�!");
    }
    public void ReplicationUpdate(ReplicationHeader rh, InputMemoryBitStream inStream)
    {
        uint networkId = rh.Nid, classId = rh.Cid;

        Debug.Log("RA_Update ���� �Ϸ�!");
        // LinkingContext �ʿ�. networkId�� �ش��ϴ� ������ ã��

        GameObject gameObject = _linkingContext.GetGameObject(networkId);
        if (null == gameObject)
        {
            // ���ο� gameObject�� �����ؼ� �����ؾ� �ϴµ�, 
            // classID�� �������� ����� Factory �Լ�? �� �־�� �Ѵ�.
            gameObject = Instantiate(Resources.Load<GameObject>("Prefabs/Shuttlecock"));
            _linkingContext.AddGameObject(networkId, gameObject);
        }
        
        // gameObject�� Read ȣ��
        NetAction netAction = gameObject.GetComponent<NetAction>();
        netAction.Read(inStream);
    }
    public void ReplicationDelete()
    {
        Debug.Log("RA_Delete ���� �Ϸ�!");
    }

    private LinkingContext _linkingContext;

    private ReplicationManager()
    {
        _linkingContext = LinkingContext.Instance;
    }

    private void Awake()
    {
        if (_instance != null && _instance != this)
        {
            Destroy(gameObject);
            return;
        }

        _instance = this;
        DontDestroyOnLoad(gameObject);
    }
}
