using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BadmintonNet : MonoBehaviour
{
    private void OnCollisionEnter2D(Collision2D collision)
    {
        if (collision.collider.CompareTag("Shuttlecock"))
        {
            Debug.Log("[Collision] BadmintonNet");

            // Net���� Shuttlecock�� linearVelocity�� �ٷ� �����ϴ� �� �����ұ�?
            Rigidbody2D rb = collision.collider.GetComponent<Rigidbody2D>();
            //if ((collision.transform.position.x - transform.position.x) * rb.linearVelocityX < 0f)
            //    rb.linearVelocityX *= -0.1f;

            rb.linearVelocityY *= 0.5f;
        }
    }

    //private void OnTriggerEnter2D(Collider2D collision)
    //{
    //    if (collision.CompareTag("Shuttlecock"))
    //    {
    //        Debug.Log("[Collision] BadmintonNet");

    //        // Net���� Shuttlecock�� linearVelocity�� �ٷ� �����ϴ� �� �����ұ�?
    //        Rigidbody2D rb = collision.GetComponent<Rigidbody2D>();
    //        if ((collision.transform.position.x - transform.position.x) * rb.linearVelocityX < 0f)
    //            rb.linearVelocityX *= -0.1f;

    //        rb.linearVelocityY *= 0.5f;
    //    }
    //}
}
