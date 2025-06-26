using UnityEngine;

[CreateAssetMenu(fileName = "RacketDatabase", menuName = "Scriptable Objects/Badminton/RacketDatabase")]
public class RacketDatabase : ScriptableObject
{
    public RacketData[] rackets;

    public RacketData GetRacketBy(string name)
    {
        foreach (var racket in rackets)
        {
            if (racket.racketName == name)
                return racket;
        }

        Debug.LogWarning($"���� {name} ��(��) ã�� �� �����ϴ�.");

        RacketData defaultRacket = rackets[0];
        return defaultRacket;
    }
}
