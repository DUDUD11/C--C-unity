using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;
using UnityEngine.UI;

public class Text_Scrollbar : MonoBehaviour
{

    public TMP_Text text;
    private int len_saver;
    RectTransform rectTransform;
    public int size;
    public Scrollbar scrollbar;
    public bool move;



    // Start is called before the first frame update
    void Start()
    {

        len_saver = 0;
        rectTransform = text.GetComponent<RectTransform>();

    }

    void Update()
    {

        int len_chk = 0;

        for (int i = 0; i < text.text.Length; i++)
        {
            if (text.text[i] == '\n')
            {
                len_chk++;
            }

        }

        rectTransform.sizeDelta = new Vector2(200, Mathf.Max(50, size * len_chk));

        if (len_saver != len_chk)
        {
            len_saver = len_chk;
            if (move)
            {
                scrollbar.value = 0;
            }
            //   scrollbar.
        }


    }
}

