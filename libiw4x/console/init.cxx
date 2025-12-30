#include <libiw4x/console/init.hxx>

#include <iostream>

using namespace std;

namespace iw4x
{
  namespace console
  {
    namespace
    {
      auto& draw (R_AddCmdDrawStretchPic);

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
      cmd_function_s Con_Clear_f_VAR;
      cmd_function_s Con_Echo_f_VAR;

      dvar_t* con_matchPrefixOnly (nullptr);
      int con_inputMaxMatchesShown (18);
      bool con_ignoreMatchPrefixOnly (false);

      constexpr int default_console_width (78);

      constexpr int virtual_screen_width (640);
      constexpr int virtual_screen_height (480);

      constexpr int tiny_char_width (8);
      constexpr int tiny_char_height (8);

      constexpr int small_char_width (8);
      constexpr int small_char_height (16);

      constexpr int big_char_width (16);
      constexpr int big_char_height (16);

      constexpr int giant_char_width (32);
      constexpr int giant_char_height (48);

      constexpr float con_inputBoxPad (6.0f);
      constexpr float con_screenPadding (4.0f);

      string build_string;
      string build_number;

      dvar_t* con_inputBoxColor (nullptr);
      dvar_t* con_outputBarColor (nullptr);
      dvar_t* con_inputHintBoxColor (nullptr);
      dvar_t* con_outputSliderColor (nullptr);
      dvar_t* con_outputWindowColor (nullptr);

      dvar_t* sa_horizontal (nullptr);
      dvar_t* sa_vertical (nullptr);
      dvar_t* sa_adjusted_horz (nullptr);
      dvar_t* sa_adjusted_vert (nullptr);

      Font_s* font_console (nullptr);

      Material* material_white (nullptr);
      Material* material_console (nullptr);

      vec4_t color_white ({1.0f, 1.0f, 1.0f, 1.0f});
      vec4_t color_title ({0.3f, 0.7f, 0.3f, 1.0f});
      vec4_t color_accent ({0.96f, 0.8f, 0.39f, 1.0f});
      vec4_t color_background ({0.03, 0.03f, 0.03f, 1.0f});
      vec4_t con_versionColor ({1.0f, 1.0f, 0.0f, 1.0f});
      vec4_t con_inputDvarMatchColor ({1.0, 1.0, 0.8, 1.0});

      namespace experimental
      {
        void
        ScrPlace_SetupFullscreenViewports ()
        {
          int displayHeight (cls->vidConfig.displayHeight);
          int displayWidth (cls->vidConfig.displayWidth);

          float safe_horz (sa_horizontal->current.value);
          float safe_vert (sa_vertical->current.value);
          float adjust_horz (sa_adjusted_horz->current.value);
          float adjust_vert (sa_adjusted_vert->current.value);

          Dvar_SetFloat (sa_horizontal, 1.0f);
          Dvar_SetFloat (sa_vertical, 1.0f);
          Dvar_SetFloat (sa_adjusted_horz, 1.0f);
          Dvar_SetFloat (sa_adjusted_vert, 1.0f);

          ScrPlace_SetupFloatRenderTargetViewport (
            scaleVirtualToReal,
            0.0,
            0.0,
            displayWidth,
            displayHeight,
            cls->vidConfig.displayWidth,
            cls->vidConfig.displayHeight);

          Dvar_SetFloat (sa_horizontal, safe_horz);
          Dvar_SetFloat (sa_vertical, safe_vert);
          Dvar_SetFloat (sa_adjusted_horz, adjust_horz);
          Dvar_SetFloat (sa_adjusted_vert, adjust_vert);

          ScrPlace_SetupFloatRenderTargetViewport (
            scrPlaceFull->scaleVirtualToReal.v,
            0.0,
            0.0,
            cls->vidConfig.displayWidth,
            cls->vidConfig.displayHeight,
            cls->vidConfig.displayWidth,
            cls->vidConfig.displayHeight);
        }

        inline void
        draw_text (const char* text, float x, float y, vec4_t* color)
        {
          // R_AddCmdDrawTextInternal (text,
          //                           0x7FFFFFFF,
          //                           cls->consoleFont,
          //                           x,
          //                           y,
          //                           1.0,
          //                           1.0,
          //                           0.0,
          //                           color->v,
          //                           0);

          R_AddCmdDrawTextInternal (text,
                                    0x7FFFFFFF,
                                    font_console,
                                    x,
                                    y,
                                    1.0f,
                                    1.0f,
                                    0.0f,
                                    color->v,
                                    0);
        }
      }

      // Done
      //
      void
      SCR_DrawSmallStringExt (int x, int y, const char* s, vec4_t* c)
      {
        Font_s* f (cls->consoleFont);
        int h (R_TextHeight (f));

        R_AddCmdDrawTextInternal (s,
                                  0x7FFFFFFF,
                                  f,
                                  x,
                                  h + y,
                                  1.0,
                                  1.0,
                                  0.0,
                                  c->v,
                                  0);
      }

      // Done
      //
      void
      Con_DrawOutputVersion (float x, float y, float w, float h)
      {
        const char* build_number (Com_GetBuildString ());
        const char* string (va ("Build %s %s", build_number, "WinX64"));
        SCR_DrawSmallStringExt (x, ((w - 18.0) + y), string, &con_versionColor);
      }

      void
      Con_DrawOutputScrollBar (float x, float y, float w, float h)
      {
        float xa (x + w - 10.0f);
        vec4_t c (con_outputBarColor->current.vector);
        ConDraw_Box (xa, y, 10.0f, h, &c);
        // [...]
      }

      void
      Con_DrawOutputText (float x, float y)
      {
        int ys (y);
        int line_count (con->visibleLineCount);
        int line_current (con->displayLineOffset - line_count);

        if (line_current < 0)
        {
          line_count = con->displayLineOffset;
          ys = y - (line_current * con->fontHeight);
          line_current = 0;
        }

        if (line_count > 0)
        {
          for (int l (0); l <= line_count; l++)
          {
            int ya (ys + con->fontHeight);

            // R_AddCmdDrawConsoleTextInternal (
            //   con->consoleWindow.circularTextBuffer,
            //   con->consoleWindow.textBufSize,
            //   con->consoleWindow
            //     .lines [(line_current + l +
            //     con->consoleWindow.firstLineIndex)
            //     %
            //             con->consoleWindow.lineCount]
            //     .textBufPos,
            //   con->consoleWindow
            //     .lines [(line_current + l +
            //     con->consoleWindow.firstLineIndex)
            //     %
            //             con->consoleWindow.lineCount]
            //     .textBufSize,
            //   cls->consoleFont,
            //   x,
            //   ya,
            //   1.0,
            //   1.0,
            //   &c,
            //   0);

            ys = ya;
          }
        }
      }

      // This is the expanded console
      //
      void
      Con_DrawOutputWindow ()
      {
        float x (con->screenMin.x);
        float y (con->screenMin.y + 32.0);
        float w (con->screenMax.x - con->screenMin.x);
        float h (con->screenMax.y - con->screenMin.y - 32.0);
        vec4_t c (con_outputWindowColor->current.vector);

        ConDraw_Box (x, y, w, h, &c);

        x += 6.0;
        y += 6.0;
        w -= 12.0;
        h -= 12.0;
        c = con_versionColor;

        Con_DrawOutputVersion (x, y, w, h);
        Con_DrawOutputScrollBar (x, y, w, h);
        Con_DrawOutputText (x, y);
      }

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
      Con_ToggleConsole ()
      {
        Field_Clear (&g_consoleField);

        if (conDrawInputGlob.matchIndex >= 0 &&
            conDrawInputGlob.autoCompleteChoice [0])
        {
          conDrawInputGlob.matchIndex = -1;
          conDrawInputGlob.autoCompleteChoice [0] = '\0';
        }

        con->outputVisible = false;
        g_consoleField.fixedSize = true;
        g_consoleField.charHeight = *g_console_char_height;
        g_consoleField.widthInPixels = *g_console_field_width;
        clientUIActives->keyCatchers ^= KEYCATCH_CONSOLE;
      }

      void
      Con_Clear_f ()
      {
        con->lineOffset = 0;
        con->displayLineOffset = 0;
      }

      void
      Con_Echo_f ()
      {
        Con_ToggleConsole ();

        I_strncpyz (g_consoleField.buffer, "\\echo ", 256);

        g_consoleField.cursor = strlen (g_consoleField.buffer);

        Field_AdjustScroll (scrPlaceFull, &g_consoleField);
      }

      // Done
      //
      void
      ConDraw_Box (float x, float y, float w, float h, vec4_t* c)
      {
        if (c->a == 0.0f)
          return;

        draw (x, y, w, h, 1.0, 1.0, 0.0, 0.0, c->v, material_white);

        float border (2.0f);
        vec4_t dark ({c->r * 0.5f, c->g * 0.5f, c->b * 0.5f, c->a});

        draw (x, y, border, h, 1.0, 1.0, 0.0, 0.0, dark.v, material_white);
        draw (x + w - border,
              y,
              border,
              h,
              1.0,
              1.0,
              0.0,
              0.0,
              dark.v,
              material_white);
        draw (x, y, w, border, 1.0, 1.0, 0.0, 0.0, dark.v, material_white);
        draw (x,
              y + h - border,
              w,
              border,
              1.0,
              1.0,
              0.0,
              0.0,
              dark.v,
              material_white);
      }

      // Done
      //
      void
      ConDrawInput_Box (int lines, vec4_t* c)
      {
        float x (conDrawInputGlob.x - 6.0);
        float y (conDrawInputGlob.y - 6.0);
        float w (conDrawInputGlob.x - 6.0);
        float h (conDrawInputGlob.fontHeight);

        w -= con->screenMin.x;
        w = (con->screenMax.x - con->screenMin.x) - w;

        h *= lines;
        h += 12.0;

        ConDraw_Box (x, y, w, h, c);
      }

      // Done
      //
      void
      ConDrawInput_Text (const char* str, vec4_t* color)
      {
        R_AddCmdDrawTextInternal (
          str,
          0x7FFFFFFF,
          cls->consoleFont,
          conDrawInputGlob.x,
          (conDrawInputGlob.fontHeight + conDrawInputGlob.y),
          1.0,
          1.0,
          0.0,
          color->v,
          0);
      }

      // Done
      //
      void
      ConDrawInput_TextAndOver (const char* str, vec4_t* color)
      {
        ConDrawInput_Text (str, color);
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

        ConDrawInput_Box (1, &con_inputBoxColor->current.vector);
        ConDrawInput_TextAndOver (version, &con_versionColor);
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
            Con_DrawAutoCompleteChoice (localClientNum, dvars_only, s);
            return;
          }
        }
        else
        {
          conDrawInputGlob.matchIndex = -1;
        }

        Con_DrawInputPrompt (localClientNum);
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

          ConDrawInput_Box (1, &con_inputHintBoxColor->current.vector);
          ConDrawInput_Text (va (warn, count), con_inputDvarMatchColor);
          Cmd_EndTokenizedString ();
          return;
        }

        if (count == 1 || (exact && Con_AnySpaceAfterCommand ()))
        {
          Dvar_ForEachName (ConDrawInput_DetailedDvarMatch);

          if (!dvars_only)
          {
            Cmd_ForEach (ConDrawInput_DetailedCmdMatch);
          }

          Cmd_EndTokenizedString ();
          return;
        }

        ConDrawInput_Box (count, &con_inputHintBoxColor->current.vector);
        Dvar_ForEachName (ConDrawInput_DvarMatch);

        if (!dvars_only)
        {
          Cmd_ForEach (ConDrawInput_Match);
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

          Dvar_ForEachName (ConDrawInput_IncrMatchCounter);

          if (!dvar_cmd)
            Cmd_ForEach (ConDrawInput_IncrMatchCounter);

          if (conDrawInputGlob.matchCount == 0)
          {
            Con_DrawInputPrompt (localClientNum);
            Cmd_EndTokenizedString ();
            return;
          }
        }

        con_ignoreMatchPrefixOnly = true;

        Dvar_ForEachName (ConDrawInput_IncrMatchCounter);

        if (!dvar_cmd)
          Cmd_ForEach (ConDrawInput_IncrMatchCounter);

        if (conDrawInputGlob.matchCount > con_inputMaxMatchesShown)
        {
          conDrawInputGlob.matchCount = 0;
          conDrawInputGlob.hasExactMatch = false;

          con_ignoreMatchPrefixOnly = false;

          Dvar_ForEachName (ConDrawInput_IncrMatchCounter);
          Cmd_ForEach (ConDrawInput_IncrMatchCounter);

          if (conDrawInputGlob.matchCount > 0)
          {
            draw_auto_complete_choice (localClientNum, dvar_cmd, input);
            draw_match_results (dvar_cmd);
            return;
          }

          conDrawInputGlob.matchCount = 0;
          conDrawInputGlob.hasExactMatch = false;

          con_ignoreMatchPrefixOnly = true;

          Dvar_ForEachName (ConDrawInput_IncrMatchCounter);

          if (!dvar_cmd)
            Cmd_ForEach (ConDrawInput_IncrMatchCounter);

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
      Con_DrawSolidConsole (int localClientNum)
      {
        Sys_EnterCriticalSection (CRITSECT_CONSOLE);

        if (con->lineOffset)
        {
          // Con_LineFeed(localClientNum, con->prevChannel, 0);
        }

        Sys_LeaveCriticalSection (CRITSECT_CONSOLE);

        con->outputVisible &=
          Key_IsCatcherActive (localClientNum, KEYCATCH_CONSOLE);

        if (con->outputVisible)
        {
          Con_DrawOutputWindow ();
        }

        Con_DrawInput (localClientNum);
      }

      void
      Con_DrawConsole (int localClientNum)
      {
        Con_CheckResize ();

        if (Key_IsCatcherActive (localClientNum, KEYCATCH_CONSOLE))
        {
          Con_DrawSolidConsole (localClientNum);
        }
      }

      void
      cg_draw_full_screen_ui (int localClientNum)
      {
        CG_DrawFullScreenUI (localClientNum);
        Con_DrawConsole (localClientNum);
      }

      void
      con_init ()
      {
        // Call original function first
        //
        Con_Init ();

        Field_Clear (&g_consoleField);

        g_consoleField.fixedSize = true;
        g_consoleField.charHeight = *g_console_char_height;
        g_consoleField.widthInPixels = *g_console_field_width;

        conDrawInputGlob.matchIndex = -1;

        Cmd_AddCommandInternal ("clear", Con_Clear_f, &Con_Clear_f_VAR);
        Cmd_AddCommandInternal ("con_echo", Con_Echo_f, &Con_Echo_f_VAR);
      }

    }

    void
    init (scheduler& s)
    {
      // In the x64 version, the API for drawing the in-game console is
      // entirely stripped out.
      //
      // Originally, this was handled during a call to CL_DrawScreen (),
      // between ScrPlace_BeginFullScreen () and ScrPlace_EndFrame ().
      // CL_DrawScreen () would check if cls.rendererStarted == true and, at
      // the end, call Con_DrawConsole () and DevGui_Draw ().
      //
      // To replicate this call sequence as closely as possible, we hook
      // CG_DrawFullScreenUI () and append our own version of
      // CG_DrawConsole(). This preserves the original conditional check and
      // maintains the correct render API order.
      //
      detour (CG_DrawFullScreenUI, &cg_draw_full_screen_ui);

      detour (Con_Init, &con_init);

      s.post ("com_frame",
              [] ()
      {
        con_matchPrefixOnly = Dvar_FindVar ("con_matchPrefixOnly");

        con_inputBoxColor = Dvar_FindVar ("con_inputBoxColor");
        con_inputHintBoxColor = Dvar_FindVar ("con_inputHintBoxColor");

        con_outputBarColor = Dvar_FindVar ("con_outputBarColor");
        con_outputWindowColor = Dvar_FindVar ("con_outputWindowColor");
        con_outputSliderColor = Dvar_FindVar ("con_outputSliderColor");

        font_console = R_RegisterFont ("fonts/consoleFont");

        material_white = Material_RegisterHandle ("white");
        material_console = Material_RegisterHandle ("console");

        Sys_ShowConsole ();

        build_string = Dvar_FindVar ("version")->current.string;
        build_number = Dvar_FindVar ("shortversion")->current.string;
        string text (format ("IW4x64 MP {} {}\n", build_number, build_string));

        Conbuf_AppendText (text.c_str ());
        cout << text << endl;

        // check sub_1400EDE50
        sa_vertical = Dvar_FindVar ("safeArea_vertical");
        sa_horizontal = Dvar_FindVar ("safeArea_horizontal");
        sa_adjusted_vert = Dvar_FindVar ("safeArea_adjusted_vertical");
        sa_adjusted_horz = Dvar_FindVar ("safeArea_adjusted_horizontal");
      });
    }
  }
}
