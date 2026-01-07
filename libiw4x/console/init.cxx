#include <libiw4x/console/init.hxx>

#include <iostream>

using namespace std;

namespace iw4x
{
  namespace console
  {
    namespace
    {
      auto& rect (R_AddCmdDrawStretchPic);
      auto& text (R_AddCmdDrawTextInternal);

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

      cmd_function_s Con_Echo_f_VAR;
      cmd_function_s Con_Clear_f_VAR;

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

      void
      ConDraw_Box (float x, float y, float w, float h, const float* c)
      {
        if (c [3] == 0.0)
          return;

        float s0 (1.0);
        float t0 (1.0);
        float s1 (0.0);
        float t1 (0.0);
        Material* m (material_white);

        rect (x, y, w, h, s0, t0, s1, t1, c, m);

        float b (2.0f);
        float dark [4](c [0] * 0.5f, c [1] * 0.5f, c [2] * 0.5f, c [3]);

        rect (x, y, b, h, s0, t0, s1, t1, dark, m);
        rect (x + w - b, y, b, h, s0, t0, s1, t1, dark, m);
        rect (x, y, w, b, s0, t0, s1, t1, dark, m);
        rect (x, y + h - b, w, b, s0, t0, s1, t1, dark, m);
      }

      void
      ConDrawInput_Box (int lines, const float* c)
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

      void
      ConDrawInput_Text (const char* s, const float* c)
      {
        int max_chars (0x7FFFFFFF);
        Font_s* f (cls->consoleFont);
        float x (conDrawInputGlob.x);
        float y (conDrawInputGlob.fontHeight + conDrawInputGlob.y);
        float xs (1.0);
        float ys (1.0);
        float ro (0.0);
        int style (0);

        text (s, max_chars, f, x, y, xs, ys, ro, c, style);
      }

      void
      R_AddCmdDrawConsoleText (const char* textPool,
                               int poolSize,
                               int firstChar,
                               int charCount,
                               Font_s* font,
                               float x,
                               float y,
                               float xScale,
                               float yScale,
                               const float* color,
                               int style)
      {
        AddBaseDrawConsoleTextCmd (textPool,
                                   poolSize,
                                   firstChar,
                                   charCount,
                                   font,
                                   x,
                                   y,
                                   xScale,
                                   yScale,
                                   color,
                                   style);
      }

      void
      SCR_DrawSmallStringExt (int x, int y, const char* s, vec4_t* c)
      {
        Font_s* f (cls->consoleFont);
        int h (R_TextHeight (f));

        text (s, 0x7FFFFFFF, f, x, h + y, 1.0, 1.0, 0.0, c->v, 0);
      }

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
        float xa (x + w - 10.0);
        float cb [4](con_outputBarColor->current.value);
        float cs [4](con_outputSliderColor->current.value);

        ConDraw_Box (xa, y, 10.0, h, cb);

        int active_count (con->consoleWindow.activeLineCount);
        int visible_count (con->visibleLineCount);

        if (active_count > visible_count)
        {
          float s (1.0 / (active_count - visible_count));
          int first_row (s * (con->displayLineOffset - visible_count));

          if ((first_row - 1.0) >= 0.0)
          {
            first_row = 1.0;
          }

          if ((0.0 - first_row) >= 0.0)
          {
            first_row = 0.0;
          }

          float ha (ceil (((s * visible_count) * h)));

          if ((10.0 - ha) >= 0.0)
          {
            ha = 10.0;
          }

          float ya (((((y + h) - ha) - y) * first_row) + y);
          ConDraw_Box (xa, ya, 10.0, h, cs);
        }
        else
        {
          ConDraw_Box (xa, y, 10.0, h, cs);
        }
      }

      void
      Con_DrawOutputText (float x, float y, float w, float h)
      {
        float c [4];
        CL_LookupColor (0, 55, c); // 0 0 0 0 0 0 128 63
        cout << "CL_LookupColor: " << c[0] << " " << c[1]<< " "<< c[2] << " " << c[3] << endl;

        int row_count (con->visibleLineCount);
        int first_row (con->displayLineOffset - row_count);

        if (first_row < 0)
        {
          y -= con->fontHeight * first_row;
          row_count = con->displayLineOffset;
          first_row = 0;
        }

        for (int row_index (0); row_index < row_count; ++row_index)
        {
          int first_line_index (con->consoleWindow.firstLineIndex);
          int line_index (row_index + first_row + first_line_index);

          line_index %= con->consoleWindow.lineCount;
          y += con->fontHeight;

          const char* txt (con->consoleWindow.circularTextBuffer);
          int s (con->consoleWindow.textBufSize);
          int first (con->consoleWindow.lines [line_index].textBufPos);
          int cnt (con->consoleWindow.lines [line_index].textBufSize);
          Font_s* f (cls->consoleFont);

          R_AddCmdDrawConsoleText (txt, s, first, cnt, f, x, y, 1.0, 1.0, c, 0);
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

        ConDraw_Box (x, y, w, h, c.v);

        x += 6.0;
        y += 6.0;
        w -= 12.0;
        h -= 12.0;
        c = con_versionColor;

        Con_DrawOutputVersion (x, y, w, h);
        Con_DrawOutputScrollBar (x, y, w, h);
        Con_DrawOutputText (x, y, w, h);
      }

      void
      Con_DrawSolidConsole (int localClientNum)
      {
        con->outputVisible &= Key_IsCatcherActive (localClientNum, 1);

        //if (con->outputVisible)
        {
          Con_DrawOutputWindow ();
        }

        // Con_DrawInput (localClientNum);
      }

      void
      Con_DrawConsole (int localClientNum)
      {
        Con_CheckResize ();

        // if (Key_IsCatcherActive (localClientNum, KEYCATCH_CONSOLE))
          Con_DrawSolidConsole (localClientNum);
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

      // Command functions
      //

      void
      Con_Echo_f ()
      {
        Con_ToggleConsole ();

        I_strncpyz (g_consoleField.buffer, "\\echo ", 256);

        g_consoleField.cursor = strlen (g_consoleField.buffer);

        Field_AdjustScroll (scrPlaceFull, &g_consoleField);
      }

      void
      Con_Clear_f ()
      {
        con->lineOffset = 0;
        con->displayLineOffset = 0;
      }

      // Hooks
      //

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

        // Add missing (stripped) code
        //

        Field_Clear (&g_consoleField);

        g_consoleField.fixedSize = true;
        g_consoleField.charHeight = *g_console_char_height;
        g_consoleField.widthInPixels = *g_console_field_width;

        conDrawInputGlob.matchIndex = -1;

        Cmd_AddCommandInternal ("clear", Con_Clear_f, &Con_Clear_f_VAR);
        Cmd_AddCommandInternal ("con_echo", Con_Echo_f, &Con_Echo_f_VAR);

        Conbuf_AppendText ("console initialized\n");
      }
    }

    void
    external_console ()
    {
      build_string = Dvar_FindVar ("version")->current.string;
      build_number = Dvar_FindVar ("shortversion")->current.string;
      string text (format ("IW4x64 MP {} {}\n", build_number, build_string));

      Sys_ShowConsole ();
      Conbuf_AppendText (text.c_str ());
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
        con_inputBoxColor = Dvar_FindVar ("con_inputBoxColor");
        con_inputHintBoxColor = Dvar_FindVar ("con_inputHintBoxColor");

        con_outputBarColor = Dvar_FindVar ("con_outputBarColor");
        con_outputWindowColor = Dvar_FindVar ("con_outputWindowColor");
        con_outputSliderColor = Dvar_FindVar ("con_outputSliderColor");

        font_console = R_RegisterFont ("fonts/consoleFont");

        material_white = Material_RegisterHandle ("white");
        material_console = Material_RegisterHandle ("console");

        // check sub_1400EDE50
        sa_vertical = Dvar_FindVar ("safeArea_vertical");
        sa_horizontal = Dvar_FindVar ("safeArea_horizontal");
        sa_adjusted_vert = Dvar_FindVar ("safeArea_adjusted_vertical");
        sa_adjusted_horz = Dvar_FindVar ("safeArea_adjusted_horizontal");

        external_console ();
      });
    }
  }
}
