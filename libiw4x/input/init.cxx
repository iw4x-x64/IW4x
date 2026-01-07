#include <libiw4x/input/init.hxx>
#include <libiw4x/console/init.hxx>

using namespace std;

namespace iw4x
{
  namespace input
  {
    namespace
    {
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

      ConDrawInputGlob conDrawInputGlob;
      field_t g_consoleField;

      dvar_t* con_matchPrefixOnly (nullptr);
      bool con_ignoreMatchPrefixOnly (false);
      int con_inputMaxMatchesShown (18);

      // void
      // Dvar_ForEachName (void (*callback) (const char*))
      // {
      //   int dvar_count;
      //   const char*** dvar_modified_flags;

      // Sys_LockRead (&g_dvarCritSect);

      // if (!areDvarsSorted)
      //   Dvar_Sort ();

      // dvar_count = 0;
      // if (dvarCount > 0)
      // {
      //   dvar_modified_flags = &sv_dvar_modifiedFlags;
      //   do
      //   {
      //     callback (**++dvar_modified_flags);
      //     ++dvar_count;
      //   }
      //   while (dvar_count < dvarCount);
      // }

      // Sys_UnlockRead (&g_dvarCritSect);
      // }

      bool
      Con_IsDvarCommand (const char* cmd)
      {
        return !I_stricmp (cmd, "set") || !I_stricmp (cmd, "seta") ||
          !I_stricmp (cmd, "sets") || !I_stricmp (cmd, "reset") ||
          !I_stricmp (cmd, "toggle") || I_stricmp (cmd, "togglep") == 0;
      }

      bool
      Con_IsAutoCompleteMatch (const char* query,
                               const char* matchToText,
                               int matchTextLen)
      {
        if (!query || !*query)
          return false;

        const bool prefix_only (con_matchPrefixOnly->current.enabled);

        if (prefix_only && !con_ignoreMatchPrefixOnly)
          return I_strnicmp (query, matchToText, matchTextLen) == 0;

        if (!prefix_only)
          return false;

        int i (0);

        for (const char* q = query; *q && i < matchTextLen; ++q)
        {
          if (tolower (*q) == tolower (matchToText [i]))
            ++i;
        }

        return i == matchTextLen;
      }

      bool
      Con_AnySpaceAfterCommand ()
      {
        const char* s (g_consoleField.buffer);

        if (!s || !*s)
          return false;

        while (isspace (*s))
          ++s;

        while (*s && !isspace (*s))
          ++s;

        return *s && isspace (*s);
      }

      void
      ConDrawInput_IncrMatchCounter (const char* str)
      {
        if (Con_IsAutoCompleteMatch (str,
                                     conDrawInputGlob.inputText,
                                     conDrawInputGlob.inputTextLen))
        {
          int matchCount (conDrawInputGlob.matchCount);

          if (conDrawInputGlob.matchCount == conDrawInputGlob.matchIndex)
          {
            I_strncpyz (conDrawInputGlob.autoCompleteChoice, str, 64);
            matchCount = conDrawInputGlob.matchCount;
          }

          conDrawInputGlob.matchCount = matchCount + 1;

          if (!str [conDrawInputGlob.inputTextLen])
            conDrawInputGlob.hasExactMatch = 1;
        }
      }

      void
      ConDrawInput_TextAndOver (const char* str, vec4_t* color)
      {
        // ConDrawInput_Text (str, color);
        conDrawInputGlob.x +=
          R_TextWidth (str, 0, cls->consoleFont) + conDrawInputGlob.x;
      }

      void
      draw_version ()
      {
        const char* version (va ("%s: %s> ", "IW4_DEV MP", "(Beta)"));

        conDrawInputGlob.x = con->screenMin.x + 6.0;
        conDrawInputGlob.y = con->screenMin.y + 6.0;
        conDrawInputGlob.leftX = con->screenMin.x + 6.0;
        conDrawInputGlob.fontHeight = R_TextHeight (cls->consoleFont);

        // ConDrawInput_Box (1, &con_inputBoxColor->current.vector);
        // ConDrawInput_TextAndOver (version, &con_versionColor);
      }

      void
      draw_auto_complete_choice (int localClientNum,
                                 bool dvars_only,
                                 const char* s)
      {
        int index (conDrawInputGlob.matchIndex);
        int count (conDrawInputGlob.matchCount);
        char c (conDrawInputGlob.autoCompleteChoice [0]);

        if (index < count && c)
        {
          if (index >= 0)
          {
            // Con_DrawAutoCompleteChoice (localClientNum, dvars_only, s);
            return;
          }
        }
        else
        {
          conDrawInputGlob.matchIndex = -1;
        }

        // Con_DrawInputPrompt (localClientNum);
      }

      void
      draw_match_results (bool dvars_only)
      {
        int count (conDrawInputGlob.matchCount);
        bool exact (conDrawInputGlob.hasExactMatch);

        conDrawInputGlob.x = conDrawInputGlob.leftX;
        conDrawInputGlob.y =
          conDrawInputGlob.y + 2 * conDrawInputGlob.fontHeight;

        if (count > con_inputMaxMatchesShown)
        {
          const char*
          warn (
            "%i matches (too many to show here, press shift+tilde to open "
            "full " "console)");

          // ConDrawInput_Box (1, &con_inputHintBoxColor->current.vector);
          // ConDrawInput_Text (va (warn, count), con_inputDvarMatchColor);
          Cmd_EndTokenizedString ();
          return;
        }

        if (count == 1 || (exact && Con_AnySpaceAfterCommand ()))
        {
          // Dvar_ForEachName (ConDrawInput_DetailedDvarMatch);

          if (!dvars_only)
          {
            // Cmd_ForEach (ConDrawInput_DetailedCmdMatch);
          }

          Cmd_EndTokenizedString ();
          return;
        }

        // ConDrawInput_Box (count, &con_inputHintBoxColor->current.vector);
        // Dvar_ForEachName (ConDrawInput_DvarMatch);

        if (!dvars_only)
        {
          // Cmd_ForEach (ConDrawInput_Match);
        }

        Cmd_EndTokenizedString ();
      }

      int
      Cmd_Argc ()
      {
        return cmd_args->argc [cmd_args->nesting];
      }

      char*
      Con_TokenizeInput ()
      {
        char* value (nullptr);

        Cmd_TokenizeString (g_consoleField.buffer);

        if (Cmd_Argc () > 0)
        {
          const char* value (*cmd_args->argv [cmd_args->nesting]);

          if (*value == '\\' || *value == '/')
            ++value;

          while (isspace (*value))
            ++value;
        }

        return value;
      }

      void
      Con_CancelAutoComplete ()
      {
        if (conDrawInputGlob.matchIndex < 0 ||
            !conDrawInputGlob.autoCompleteChoice [0])
          return;

        conDrawInputGlob.matchIndex = -1;
        conDrawInputGlob.autoCompleteChoice [0] = 0;
      }

      void
      Con_DrawInputPrompt (int localClientNum)
      {
        Field_Draw (localClientNum,
                    &g_consoleField,
                    conDrawInputGlob.x,
                    conDrawInputGlob.y,
                    5,
                    5);
      }

      void
      Con_DrawInput (int localClientNum)
      {
        if (!Key_IsCatcherActive (localClientNum, 1) || !Sys_IsMainThread ())
          return;

        draw_version ();

        g_consoleField.widthInPixels = con->screenMax.x - 6.0;
        g_consoleField.widthInPixels -= conDrawInputGlob.x;

        conDrawInputGlob.leftX = conDrawInputGlob.x;
        conDrawInputGlob.autoCompleteChoice [0] = '\0';

        int prev_len (conDrawInputGlob.inputTextLen);
        const char* input (Con_TokenizeInput ());

        conDrawInputGlob.inputText = input;
        conDrawInputGlob.inputTextLen = strlen (input);

        if (prev_len != conDrawInputGlob.inputTextLen)
        {
          Con_CancelAutoComplete ();
        }

        if (conDrawInputGlob.inputTextLen == 0)
        {
          conDrawInputGlob.mayAutoComplete = false;
          Con_DrawInputPrompt (localClientNum);
          Cmd_EndTokenizedString ();
          return;
        }

        const bool dvar_cmd (Cmd_Argc () > 1 && Con_IsDvarCommand (input));

        if (dvar_cmd)
        {
          conDrawInputGlob.inputText = Cmd_Argv (1);
          conDrawInputGlob.inputTextLen = strlen (conDrawInputGlob.inputText);

          if (conDrawInputGlob.inputTextLen == 1)
          {
            conDrawInputGlob.mayAutoComplete = false;
            Con_DrawInputPrompt (localClientNum);
            Cmd_EndTokenizedString ();
            return;
          }
        }

        conDrawInputGlob.matchCount = 0;
        conDrawInputGlob.hasExactMatch = false;

        if (!con_matchPrefixOnly->current.enabled)
        {
          con_ignoreMatchPrefixOnly = false;

          // Dvar_ForEachName (ConDrawInput_IncrMatchCounter);

          if (!dvar_cmd)
            // Cmd_ForEach (ConDrawInput_IncrMatchCounter);

          if (conDrawInputGlob.matchCount == 0)
          {
            Con_DrawInputPrompt (localClientNum);
            Cmd_EndTokenizedString ();
            return;
          }
        }

        con_ignoreMatchPrefixOnly = true;

        // Dvar_ForEachName (ConDrawInput_IncrMatchCounter);

        // if (!dvar_cmd)
          // Cmd_ForEach (ConDrawInput_IncrMatchCounter);

        if (conDrawInputGlob.matchCount > con_inputMaxMatchesShown)
        {
          conDrawInputGlob.matchCount = 0;
          conDrawInputGlob.hasExactMatch = false;

          con_ignoreMatchPrefixOnly = false;

          // Dvar_ForEachName (ConDrawInput_IncrMatchCounter);
          // Cmd_ForEach (ConDrawInput_IncrMatchCounter);

          if (conDrawInputGlob.matchCount > 0)
          {
            draw_auto_complete_choice (localClientNum, dvar_cmd, input);
            draw_match_results (dvar_cmd);
            return;
          }

          conDrawInputGlob.matchCount = 0;
          conDrawInputGlob.hasExactMatch = false;

          con_ignoreMatchPrefixOnly = true;

          // Dvar_ForEachName (ConDrawInput_IncrMatchCounter);

          // if (!dvar_cmd)
            // Cmd_ForEach (ConDrawInput_IncrMatchCounter);

          if (conDrawInputGlob.matchCount == 0)
          {
            Con_DrawInputPrompt (localClientNum);
            Cmd_EndTokenizedString ();
            return;
          }
        }

        if (conDrawInputGlob.matchCount == 0)
        {
          Con_DrawInputPrompt (localClientNum);
          Cmd_EndTokenizedString ();
          return;
        }

        draw_auto_complete_choice (localClientNum, dvar_cmd, input);
        draw_match_results (dvar_cmd);
      }

      void
      cl_key_event (int localClientNum, int key, bool down, int time)
      {
        static constexpr char tilde (126);
        static constexpr char grave (96);

        if (key == tilde || key == grave)
        {
          // Con_ToggleConsole ();
        }

        CL_KeyEvent (localClientNum, key, down, time);
      }
    }

    void
    init (scheduler& s)
    {
      detour (CL_KeyEvent, &cl_key_event);

      s.post ("com_frame",
              [] ()
      {
        con_matchPrefixOnly = Dvar_FindVar ("con_matchPrefixOnly");
      });
    }
  }
}
