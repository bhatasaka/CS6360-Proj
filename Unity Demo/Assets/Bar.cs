using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;

public class Bar : MonoBehaviour
{
    float initialPos;
    const float relativeMaxHeight = 1.2f;
    //PlayerControls controls;

    void Awake()
    {
        //controls = new PlayerControls();
        //controls.Gameplay.ToggleShock.performed += (ctx) => Test();
    }
    // Start is called before the first frame update
    void Start()
    {
        initialPos = transform.position.y;
    }

    // Update is called once per frame
    void Update()
    {
        
    }
    public void Zero()
    {
        transform.position = new Vector3(transform.position.x, initialPos, transform.position.z);
    }

    /// <summary>
    /// Set a relative height given a height value
    /// between 0 and 1.
    /// </summary>
    /// <param name="relHeight"></param>
    public void SetHeight(float relHeight)
    {
        var newHeight = initialPos + (relHeight * relativeMaxHeight);
        transform.position = new Vector3(transform.position.x, newHeight, transform.position.z);
    }
}
