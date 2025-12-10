#include <libiw4x/console/init.hxx>

#include <libiw4x/renderer/init.hxx>

#include <iostream>

using namespace std;

namespace iw4x
{
  namespace console
  {
    namespace
    {
      using sub_140272D70_t = void (*) (void);
      inline sub_140272D70_t sub_140272D70 = reinterpret_cast<sub_140272D70_t> (0x140272D70);

      struct console
      {
        bool outputVisible;
        char consoleText [32768];
        int fontHeight;
        int visibleLineCount;
        int visiblePixelWidth;
        vec4_t screen;
        vec4_t color;
      };

      struct ConDrawInputGlob
      {
        char autoCompleteChoice [64];
        int matchIndex;
        int matchCount;
        const char* inputText;
        int inputTextLen;
        bool hasExactMatch;
        bool mayAutoComplete;
        float x;
        float y;
        float leftX;
        float fontHeight;
      };

      console con;
      field_t g_consoleField;

      dvar_t* con_inputBoxColor (nullptr);
      Material* white (nullptr);

      void
      con_OpenConsoleOutput ()
      {
        if (Key_IsCatcherActive (0, KEYCATCH_CONSOLE))
        {
          con.outputVisible = true;
        }
      }

      void
      con_ToggleConsoleOutput ()
      {
        con.outputVisible = !con.outputVisible;
        *keyCatchers ^= KEYCATCH_CONSOLE;
      }

      void
      cl_keyEvent (int localClientNum, int key, int down, unsigned int time)
      {
        if (key == 126 && down)
        {
          con_ToggleConsoleOutput ();
        }

        CL_KeyEvent (localClientNum, key, down, time);
      }

      void
      Con_DrawConsole ()
      {
        Con_CheckResize ();

        if (Key_IsCatcherActive (0, KEYCATCH_CONSOLE))
        {
          auto color (con_inputBoxColor->current.vector);
          vec4_t dark (color [0] * 0.5f,
                       color [1] * 0.5f,
                       color [2] * 0.5f,
                       color [3]);

          auto* viewport (ScrPlace_GetViewPlacement ()->realViewportSize);
          float screen_min [2](6.0f, 6.0f);
          float screen_max [2](viewport [0] - 6.0f, viewport [1] - 6.0f);

          float x (screen_min [0]);
          float y (screen_min [1]);
          float w (screen_max [0] - screen_min [0]);
          float h (32.0f);

          R_AddCmdDrawStretchPic (x, y, w, h, 0, 0, 0, 0, color, white);
          R_AddCmdDrawStretchPic (x, y, 2, h, 0, 0, 0, 0, dark, white);
          R_AddCmdDrawStretchPic (x + w - 2, y, 2, h, 0, 0, 0, 0, dark, white);
          R_AddCmdDrawStretchPic (x, y, w, 2, 0, 0, 0, 0, dark, white);
          R_AddCmdDrawStretchPic (x, y + h - 2, w, 2, 0, 0, 0, 0, dark, white);
        }
      }

      void
      con_init ()
      {
        Field_Clear (&g_consoleField);
        g_consoleField.widthInPixels = *g_console_field_width;
        g_consoleField.charHeight = 16.0f; // g_console_char_height;
        g_consoleField.fixedSize = 1;

        // conDrawInputGlob.matchIndex = -1;

        // Cmd_AddCommandInternal ("clear", Con_Clear_f, &Con_Clear_f_VAR);
        // Cmd_AddCommandInternal ("con_echo", Con_Echo_f, &Con_Echo_f_VAR);

        // if (!con.initialized)
        // {
        //   Con_OneTimeInit ();
        // }
      }

      // Attempts to auto-complete `prefix` using a list of candidate strings.
      // If only one string matches, `completed` becomes that full string.
      // If multiple match, `completed` becomes the longest common prefix
      // extension.
      void
      con_AutoCompleteFromList (const char** strings,
                                unsigned int stringCount,
                                const char* prefix,
                                char* completed,
                                unsigned int sizeofCompleted)
      {
        size_t prefix_len (strlen (prefix));
        completed [0] = '\0';

        for (size_t i = 0; i < stringCount; i++)
        {
          const char* candidate (strings [i]);

          // Skip non-matching entries
          //
          if (I_strnicmp (prefix, candidate, prefix_len) != 0)
            continue;

          // First match initializes `completed` with the entire candidate
          //
          if (completed [0] == '\0')
          {
            I_strncpyz (completed, candidate, sizeofCompleted);
            continue;
          }

          // Multiple matches: compute the common extension byeond `prefix_len`
          //
          size_t pos (prefix_len);

          // Only try to extend if the next character matches
          //
          if (candidate [pos] == completed [pos])
          {
            // Compute offset between candidate and completed
            //
            int shift = candidate - completed;

            // Walk forward while characters remain equal
            //
            while (completed [pos] != '\0')
            {
              char c (completed [pos + shift + 1]);

              if (c != completed [pos + 1])
                break;

              pos++;
            }
          }

          completed [pos] = '\0';
        }
      }

      void
      hook ()
      {
        auto* viewport (ScrPlace_GetViewPlacement ()->realViewportSize);

        float screen_min [2](6.0f, 6.0f); // left & top
        float screen_max [2](viewport [0] - 6.0f,
                             viewport [1] - 6.0f); // right & bottom

        float x (screen_min [0]);
        float y (screen_min [1]);
        float w (screen_max [0] - screen_min [0]);
        float h (32.0f);
        auto color = con_inputBoxColor->current.vector;

        vec4_t dark_color (color [0] * 0.5f,
                           color [1] * 0.5f,
                           color [2] * 0.5f,
                           color [3]);

        R_AddCmdDrawStretchPic (x,
                                y,
                                w,
                                h,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                color,
                                white);
        R_AddCmdDrawStretchPic (x,
                                y,
                                2.0f,
                                h,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                dark_color,
                                white);
        R_AddCmdDrawStretchPic (x + w - 2.0f,
                                y,
                                2.0f,
                                h,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                dark_color,
                                white);
        R_AddCmdDrawStretchPic (x,
                                y,
                                w,
                                2.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                dark_color,
                                white);
        R_AddCmdDrawStretchPic (x,
                                (y + h) - 2.0f,
                                w,
                                2.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                dark_color,
                                white);

        R_EndFrame ();
      }
    }

    void
    init (scheduler& s)
    {
      detour (CL_KeyEvent, &cl_keyEvent);
      detour (sub_140272D70, &Con_DrawConsole);

      s.post ("com_frame",
              [] ()
      {
        const char* build (Com_GetBuildString ());
        string message (format ("IW4 MP 2.0 build {} win64\n", build));

        Sys_ShowConsole ();
        Conbuf_AppendText (message.c_str ());

        con_inputBoxColor = Dvar_FindVar ("con_inputBoxColor");
        white = Material_RegisterHandle ("white");
      });
    }

  }
}

// Dvar_FindVar ("con_inputBoxColor"); 0x1400E99FC
// Color of the console input box
//
// dvar_t *con_inputBoxColor (nullptr);

// Dvar_FindVar ("con_inputHintBoxColor"); 0x1400E9A4D
// Color of the console input hint box
//
// dvar_t *con_inputHintBoxColor (nullptr);

// Dvar_FindVar ("con_outputBarColor"); 0x1400E9AA1
// Color of the console output slider bar
//
// dvar_t *con_outputBarColor (nullptr);

// Dvar_FindVar ("con_outputSliderColor"); 0x1400E9AEF
// Color of the console slider
//
// dvar_t *con_outputSliderColor (nullptr);

// Dvar_FindVar ("con_outputWindowColor"); 0x1400E9B41
// Color of the console output
//
// dvar_t *con_outputWindowColor (nullptr);

// Vertical safe area as a fraction of the screen height
//
// dvar_t *safeArea_vertical = Dvar_FindVar ("safeArea_vertical");
// Horizontal safe area as a fraction of the screen width
//
// dvar_t *safeArea_horizontal = Dvar_FindVar ("safeArea_horizontal");
// User-adjustable vertical safe area as a fraction of the screen height
//
// dvar_t *safeArea_adjusted_vertical = Dvar_FindVar
// ("safeArea_adjusted_vertical");
// User-adjustable horizontal safe area as a fraction of the screen width
//
// dvar_t *safeArea_adjusted_horizontal = Dvar_FindVar
// ("safeArea_adjusted_horizontal");

// // Color of the console input box
// //
// con_inputBoxColor = Dvar_FindVar ("con_inputBoxColor");
// con_inputBoxColor->current.color [0] = 0.20f;
// con_inputBoxColor->current.color [1] = 0.20f;
// con_inputBoxColor->current.color [2] = 0.20f;
// con_inputBoxColor->current.color [3] = 1.00f;

// // Color of the console input hint box
// //
// con_inputHintBoxColor = Dvar_FindVar ("con_inputHintBoxColor");
// con_inputHintBoxColor->current.color [0] = 0.30f;
// con_inputHintBoxColor->current.color [1] = 0.30f;
// con_inputHintBoxColor->current.color [2] = 0.30f;
// con_inputHintBoxColor->current.color [3] = 1.00f;

// // Color of the console output slider bar
// //
// con_outputBarColor = Dvar_FindVar ("con_outputBarColor");
// con_outputBarColor->current.color [0] = 0.50f;
// con_outputBarColor->current.color [1] = 0.50f;
// con_outputBarColor->current.color [2] = 0.50f;
// con_outputBarColor->current.color [3] = 0.60f;

// // Color of the console slider
// //
// con_outputSliderColor = Dvar_FindVar ("con_outputSliderColor");
// con_outputSliderColor->current.color [0] = 0.70f;
// con_outputSliderColor->current.color [1] = 1.00f;
// con_outputSliderColor->current.color [2] = 0.00f;
// con_outputSliderColor->current.color [3] = 1.00f;

// // Color of the console output
// //
// con_outputWindowColor = Dvar_FindVar ("con_outputWindowColor");
// con_outputWindowColor->current.color [0] = 0.25f;
// con_outputWindowColor->current.color [1] = 0.25f;
// con_outputWindowColor->current.color [2] = 0.25f;
// con_outputWindowColor->current.color [3] = 0.85f;

// material_white = Material_RegisterHandle ("white");
// // Font_s* console_font (CL_RegisterFont ("fonts/consoleFont", 0));

// detour (R_EndFrame, &hook);

// void
// console_draw_input(int localClientNum)
// {
//   bool isDvarCommand (false);
//   bool con_matchPrefixOnly (false);
//   bool con_ignoreMatchPrefixOnly (false);

// if (Key_IsCatcherActive (localClientNum, 1))
// {
//   conDrawInputGlob.fontHeight = R_TextHeight (cls.consoleFont);
//   conDrawInputGlob.leftX      = con.screenMin.v[0] + 6.0;
//   conDrawInputGlob.x          = con.screenMin.v[0] + 6.0;
//   conDrawInputGlob.y          = con.screenMin.v[1] + 6.0;

// Dvar_GetVec4 (con_inputBoxColor, &inputBoxColor);

// ConDraw_Box (&inputBoxColor,
//              conDrawInputGlob.x - 6.0,
//              conDrawInputGlob.y - 6.0,
//              (con.screenMax.v[0] - con.screenMin.v[0]) -
//              ((conDrawInputGlob.x
//              - 6.0) - con.screenMin.v[0]), conDrawInputGlob.fontHeight
//              + 12.0);

// ConDrawInput_TextAndOver (localClientNum, v2, &con_versionColor);

// conDrawInputGlob.leftX       = conDrawInputGlob.x;
// g_consoleField.widthInPixels = ((con.screenMax.v[0] - 6.0) -
// conDrawInputGlob.x);

// v3 = Con_TokenizeInput ();
// conDrawInputGlob.inputText    = v3;
// conDrawInputGlob.inputTextLen = strlen (v3);
// conDrawInputGlob.autoCompleteChoice[0] = 0;

// if (!conDrawInputGlob.inputTextLen)
// {
//   conDrawInputGlob.mayAutoComplete = 0;

// Field_Draw (localClientNum,
//             &g_consoleField,
//             conDrawInputGlob.x,
//             conDrawInputGlob.y,
//             5, 5);
// LABEL_39:
// Cmd_EndTokenizedString ();
// return;
// }

// originalCommand = v3;

// if (Cmd_Argc () > 1 && Con_IsDvarCommand (conDrawInputGlob.inputText))
// {
//   isDvarCommand = true;

// conDrawInputGlob.inputText    = Cmd_Argv (1);
// conDrawInputGlob.inputTextLen = strlen (conDrawInputGlob.inputText);

// if (!conDrawInputGlob.inputTextLen)
// {
//   conDrawInputGlob.mayAutoComplete = 0;

// LABEL_12:
//   Con_DrawInputPrompt (localClientNum);
//   goto LABEL_39;
// }
// }
// else
// {
// isDvarCommand = 0;
// }

// con_matchPrefixOnly = Dvar_GetBool (con_matchPrefixOnly);
// conDrawInputGlob.hasExactMatch = 0;
// conDrawInputGlob.matchCount    = 0;

// if (con_matchPrefixOnly)
// {
//   con_ignoreMatchPrefixOnly = true;
//   Dvar_ForEachName (ConDrawInput_IncrMatchCounter);

// if (!isDvarCommand)
// {
//   Cmd_ForEach (ConDrawInput_IncrMatchCounter);
// }

// if (conDrawInputGlob.matchCount <= con_inputMaxMatchesShown
// || (conDrawInputGlob.hasExactMatch = 0,
//     conDrawInputGlob.matchCount    = 0,
//     con_ignoreMatchPrefixOnly      = 0,
//     Dvar_ForEachName (ConDrawInput_IncrMatchCounter),
//     Cmd_ForEach (ConDrawInput_IncrMatchCounter),
//     conDrawInputGlob.matchCount))
// {
// LABEL_23:
//   matchCount = conDrawInputGlob.matchCount;

// if (!conDrawInputGlob.matchCount)
// {
//   goto LABEL_12;
// }

// if (conDrawInputGlob.matchIndex < conDrawInputGlob.matchCount &&
// conDrawInputGlob.autoCompleteChoice[0])
// {
//   if (conDrawInputGlob.matchIndex >= 0)
//   {
//     Con_DrawAutoCompleteChoice (localClientNum, isDvarCommand,
//     originalCommand);

// LABEL_28:
// conDrawInputGlob.y = (conDrawInputGlob.y + conDrawInputGlob.fontHeight) +
// conDrawInputGlob.fontHeight; conDrawInputGlob.x = conDrawInputGlob.leftX;

// Dvar_GetVec4 (con_inputHintBoxColor, &inputHintBoxColor);

// if (matchCount <= con_inputMaxMatchesShown)
// {
//   if (matchCount == 1 || conDrawInputGlob.hasExactMatch &&
//   Con_AnySpaceAfterCommand ())
//   {
//     Dvar_ForEachName (localClientNum, ConDrawInput_DetailedDvarMatch);

// if (!isDvarCommand)
// {
//   Cmd_ForEach (localClientNum, ConDrawInput_DetailedCmdMatch);
// }
// }
// else
// {
// ConDrawInput_Box (matchCount, &inputHintBoxColor);
// Dvar_ForEachName (ConDrawInput_DvarMatch);

// if (!isDvarCommand)
// {
//   Cmd_ForEach (ConDrawInput_CmdMatch);
// }
// }
// }
// else
// {
// const char* hint (va ("%i matches (too many to show here, press shift+tilde
// to open full console)", matchCount)); ConDrawInput_Box (1,
// &inputHintBoxColor); ConDrawInput_Text (hint, &con_inputDvarMatchColor);
// }

// goto LABEL_39;
// }
// }
// else
// {
// conDrawInputGlob.matchIndex = -1;
// }

// Con_DrawInputPrompt (localClientNum);
// goto LABEL_28;
// }

// conDrawInputGlob.hasExactMatch = false;
// conDrawInputGlob.matchCount    = false;
// con_ignoreMatchPrefixOnly      = true;
// }
// else
// {
// con_ignoreMatchPrefixOnly = false;
// }

// Dvar_ForEachName(ConDrawInput_IncrMatchCounter);

// if (!isDvarCommand)
// {
//   Cmd_ForEach(ConDrawInput_IncrMatchCounter);
// }

// goto LABEL_23;
// }
// }
