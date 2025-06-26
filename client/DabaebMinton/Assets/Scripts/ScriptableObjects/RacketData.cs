using UnityEngine;

[CreateAssetMenu(fileName = "RacketData", menuName = "Scriptable Objects/Badminton/RacketData")]
public class RacketData : ScriptableObject
{
    public string racketName;
    public float weight;            // �������� ������
    public float height;            // ���� ����
    public float repulsionPower;    // �ݹ߷�
    public Sprite icon;             // UI�� ������
    public Sprite sprite;           // Game Image

    // �ʿ��� �Ӽ� �� �߰� ����
}
