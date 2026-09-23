#undef NDEBUG
#include <cassert>

#include <cstring>
#include <string_view>

#include <libiw4x/gdk/text.hxx>

using namespace iw4x::gdk;

int
main ()
{
  {
    text<64> t;

    assert (t.size () == 0);
    assert (std::strcmp (t.c_str (), "") == 0);
  }

  {
    text<64> t ("{} and {}", 1, 2);

    assert (std::strcmp (t.c_str (), "1 and 2") == 0);
    assert (t.size () == 7);
    assert (std::string_view (t) != "1 and 7");
  }

  {
    text<64> t ("no arguments");

    assert (std::strcmp (t.c_str (), "no arguments") == 0);
  }

  {
    text<64> t ("{}{}{}", 1, 2, 3);

    assert (std::strcmp (t.c_str (), "123") == 0);
  }

  {
    text<64> t ("more braces {} {} {}", 1);

    assert (std::strcmp (t.c_str (), "more braces 1 {} {}") == 0);
  }

  {
    text<64> t ("fewer braces {}", 1, 2, 3);

    assert (std::strcmp (t.c_str (), "fewer braces 123") == 0);
  }

  {
    text<64> t ("{}", "text");
    assert (std::strcmp (t.c_str (), "text") == 0);

    text<64> n ("{}", static_cast<const char*> (nullptr));
    assert (std::strcmp (n.c_str (), "(null)") == 0);

    text<64> v ("{}", std::string_view ("view"));
    assert (std::strcmp (v.c_str (), "view") == 0);
  }

  {
    text<64> t ("{} {} {} {}", true, false, 'x', '\n');

    assert (std::strcmp (t.c_str (), "true false x \n") == 0);
  }

  {
    assert (std::strcmp (text<64> ("{}", 0).c_str (), "0") == 0);
    assert (std::strcmp (text<64> ("{}", -1).c_str (), "-1") == 0);
    assert (std::strcmp (text<64> ("{}", 1234567890).c_str (),
                         "1234567890") == 0);

    assert (std::strcmp (
              text<64> ("{}", static_cast<std::int64_t> (-9223372036854775807LL - 1)).c_str (),
              "-9223372036854775808") == 0);

    assert (std::strcmp (
              text<64> ("{}", static_cast<std::uint64_t> (18446744073709551615ULL)).c_str (),
              "18446744073709551615") == 0);
  }

  {
    assert (std::strcmp (text<64> ("{}", hex_number (0)).c_str (),
                         "0x0") == 0);
    assert (std::strcmp (text<64> ("{}", hex_number (0xABCDEF)).c_str (),
                         "0xABCDEF") == 0);
    assert (std::strcmp (text<64> ("{}", hex_number (0xF, 8)).c_str (),
                         "0x0000000F") == 0);
    assert (std::strcmp (text<64> ("{}",
                                   hex_number (0x123456789ABCDEF0ULL, 16))
                           .c_str (),
                         "0x123456789ABCDEF0") == 0);
  }

  {
    int  v (0);
    void* p (&v);

    text<64> t ("{}", static_cast<const void*> (p));

    assert (std::strncmp (t.c_str (), "0x", 2) == 0);
  }

  {
    text<8> t ("{}", "abcdefghijkl");

    assert (std::strcmp (t.c_str (), "abcdefg") == 0);
    assert (t.size () == 7);
  }

  {
    char        b[16];
    text_writer w (b, sizeof (b));

    assert (w.size () == 0 && w.whole ());

    w.write (std::string_view ("0123456789"));
    assert (w.whole () && w.size () == 10);

    w.write (std::string_view ("0123456789"));
    assert (!w.whole () && w.size () == 15);
    assert (b[15] == '\0');
  }

  {
    text<32> a ("value {}", 1);
    text<32> b (a);

    assert (std::strcmp (b.c_str (), "value 1") == 0);
    assert (b.size () == a.size ());
  }
}
