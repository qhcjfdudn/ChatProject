using UnityEngine;

public class PlayerRacket : MonoBehaviour
{
    [Header("Badminton Racket Settings")]
    [Tooltip("BadmintonRacket ScriptableObject�� List�� �����մϴ�.")]
    public RacketDatabase _racketDatabase;

    [Tooltip("���� ���õ� BadmintonRacket�Դϴ�. \n" +
        "Racket�� �̸��� ��Ȯ�� �Է����� ������ Default Racket�� ���õ˴ϴ�.")]
    public string _selectedRacketName;

    public SpriteRenderer _racketSpriteRenderer;

    private RacketData _currentRacket;

    public float GetRacketHeight()
    {
        return _currentRacket.height;
    }

    private void ApplyRacketStats()
    {
        Debug.Log($"[{_currentRacket.racketName}] ����� - ����: {_currentRacket.weight}, �ݹ߷�: {_currentRacket.repulsionPower}");
        // ĳ���� ���ɿ� �ݿ��ϴ� �ڵ� �ۼ�

        _racketSpriteRenderer.sprite = _currentRacket.sprite;
    }

    private void Awake()
    {
        _racketSpriteRenderer = GetComponent<SpriteRenderer>();

        _currentRacket = _racketDatabase.GetRacketBy(_selectedRacketName);
        ApplyRacketStats();
    }
}
