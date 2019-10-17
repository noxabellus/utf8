#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <climits>

// only using 3 functions so avoiding including the whole header and poluting namespace
// #define UTF8PROC_STATIC
// #include "../../extern/utf8proc/utf8proc.h"



namespace utf8 {
  /* Prepare Windows' console for UTF8 IO */
  extern void setup_console ();

  /* Get the byte size of a given grapheme */
  extern uint8_t char_size (uint8_t const* c);

  /* Get the byte size of a given grapheme */
  extern uint8_t char_size (uint32_t c);

  /* Convert a utf8 grapheme to utf32 */
  extern uint32_t to_int (uint8_t const* c);

  /* Convert a utf32 grapheme to utf8 */
  extern size_t encode (uint32_t c, uint8_t* bytes);

  /* Add a utf8 grapheme to a file */
  extern size_t put_char (uint8_t const* ustr, FILE* f);

  /* Add a utf32 grapheme to a file as utf8 */
  extern size_t put_char (uint32_t c, FILE* f);

  /* Get a utf8 grapheme from a file */
  extern size_t get_char (uint8_t* ustr, FILE* f);

  /* Get a utf8 grapheme from a file as utf32 */
  extern uint32_t get_char (FILE* f);

  /* Offset a pointer to a utf8 grapheme index */
  extern uint8_t* index_offset (uint8_t* ustr, size_t index);

  /* Offset a pointer to a utf8 grapheme index */
  extern uint8_t const* index_offset (uint8_t const* ustr, size_t index);

  /* Get the byte offset of a utf8 grapheme index */
  extern size_t byte_offset (uint8_t const* ustr, size_t index);

  /* Get the number of graphemes in a segment of utf8 */
  extern size_t char_count (uint8_t const* ustr, size_t max_byte_length = SIZE_MAX);

  /* Get the number of bytes in a utf8 ustr (wrapper for strlen) */
  inline size_t byte_count (uint8_t const* ustr) {
    return strlen((char const*) ustr);
  }

  /* Extract a grapheme at an index in a utf8 ustr as utf32 */
  extern uint32_t char_at (uint8_t const* ustr, size_t index);

  /* Determine whether a specific grapheme is a whitespace character */
  extern bool is_whitespace (uint32_t c);

  /* Determine whether a specific grapheme is a whitespace character */
  extern bool is_whitespace (uint8_t const* c);

  /* Get the number of visual columns associated with a series of utf8 graphemes */
  extern size_t column_count (uint8_t const* ustr, size_t max_byte_length = SIZE_MAX);

  /* Get the number of visual columns associated with a utf32 grapheme */
  extern size_t column_count (uint32_t c);


  /* Wrapper for index and value returned by StringIterator */
  struct StringIteratorResult {
    size_t i;
    uint32_t v;
  };


  /* Simple index + value pair iterator for String */
  struct StringIterator {
    size_t index = 0;
    uint8_t const* bytes = NULL;

    StringIteratorResult operator * () const;

    StringIterator& operator ++ ();

    bool operator != (StringIterator const& other) const;
  };


  /* Utf8 aware String representation for dynamic allocation */
  struct String {
    static constexpr size_t DEFAULT_CAPACITY = 16;

    uint8_t* bytes = NULL;
    size_t byte_length = 0;
    size_t byte_capacity = 0;

    /* Create a 0-initialized String */
    String () = default;

    /* Create a String with an initial capacity */
    String (size_t init_capacity)
    { grow_allocation(init_capacity); }

    /* Create a String from a ustr */
    String (uint8_t const* src)
    { insert(src); }

    /* Create a String from a str */
    String (char const* src)
    { insert((uint8_t const*) src); }

    /* Create a String from a ustr subsection */
    String (uint8_t const* src, size_t length)
    { insert(src, length); }

    /* Create a String from a str subsection */
    String (char const* src, size_t length)
    { insert((uint8_t const*) src, length); }

    /* Create a copy of a String */
    String (String const& src)
    { insert(src.bytes, src.byte_length); }

    /* Create a String manually by taking ownership of existing data */
    String (uint8_t* in_bytes, size_t in_byte_length, size_t in_byte_capacity)
    : bytes(in_bytes)
    , byte_length(in_byte_length)
    , byte_capacity(in_byte_capacity)
    { }

    /* Wraps dispose for automatic clean up when going out of scope */
    ~String () {
      dispose();
    }

    /* Create a StringIterator representing the start of the String */
    StringIterator begin () const {
      return { 0, bytes };
    }

    /* Create a StringIterator representing the end of the String */
    StringIterator end () const {
      return { utf8::char_count(bytes), bytes + byte_length };
    }

    /* Cast to str */
    operator char* () const { return (char*) bytes; }

    /* Cast to const str */
    operator char const* () const { return (char const*) bytes; }

    /* Cast to ustr */
    operator uint8_t* () const { return bytes; }

    /* Cast to const ustr*/
    operator uint8_t const* () const { return bytes; }

    /* Compare to ustr (Wrapper for strcmp) */
    bool operator == (uint8_t const* other) const {
      return strcmp((char const*) bytes, (char const*) other) == 0;
    }

    /* Compare to ustr (Wrapper for strcmp) */
    bool operator != (uint8_t const* other) const {
      return strcmp((char const*) bytes, (char const*) other) != 0;
    }

    /* Get the grapheme at an index in a String (Wrapper for char_at) */
    uint32_t operator [] (size_t index) const {
      return char_at(index);
    }

    /* Get the length of a String in graphemes (Wrapper for char_count) */
    size_t length () const {
      return utf8::char_count(bytes);
    }

    /* Get the grapheme at an index in a String (Wrapper for utf8::char_at) */
    uint32_t char_at (size_t index) const {
      return utf8::char_at(bytes, index);
    }

    /* Get a ustr at a grapheme index (Wrapper for index_offset) */
    uint8_t* str_at (size_t index) const {
      return utf8::index_offset(bytes, index);
    }


    /* Free dynamically allocated memory for a String and zero initialize it again */
    void dispose ();

    /* Take ownership of a String's internal data and zero initialize it again */
    uint8_t* release ();

    /* Move a String's data into another String */
    void move (String& source);


    /* Load a utf8 file by name and create a String from it */
    static String from_file (char const* file_name);

    /* Store a String to a utf8 file by name */
    void to_file (char const* file_name) const;


    /* Reduce the capacity of a String to match its length with no extra (Used by release) */
    void collapse_allocation ();

    /* Grow the capacity of a String to fit some additional length */
    void grow_allocation (size_t additional_length);


    /* Insert a ustr or subsection into a String */
    void insert (uint8_t const* str, size_t length = 0);

    /* Insert a str or subsection into a String */
    void insert (char const* str, size_t length = 0);

    /* Insert a single utf32 grapheme into a String */
    void insert (uint32_t c);


    /* Insert a ustr or subsection into a String at an existing grapheme index */
    void insert_at (size_t index, uint8_t const* seg, size_t length = 0);

    /* Insert a str or subsection into a String at an existing grapheme index */
    void insert_at (size_t index, char const* str, size_t length = 0);

    /* Insert a single utf32 grapheme into a String at an existing grapheme index */
    void insert_at (size_t index, uint32_t c);

    /* Remove a subsection (defined in byte size) from a String */
    void remove (size_t index, size_t count = 1);


    /* Wrapper for vsnprintf that appends the result to the end of a String */
    void insert_fmt_va (uint8_t const* fmt, va_list args);

    /* Wrapper for vsnprintf that appends the result to the end of a String */
    void insert_fmt_va (char const* fmt, va_list args);

    /* Wrapper for vsnprintf that appends the result to the end of a String */
    void insert_fmt (uint8_t const* fmt, ...);

    /* Wrapper for vsnprintf that appends the result to the end of a String */
    void insert_fmt (char const* fmt, ...);

    /* Wrapper for vsnprintf that inserts the result at an existing grapheme index in a String */
    void insert_fmt_at_va (size_t index, uint8_t const* fmt, va_list args);

    /* Wrapper for vsnprintf that inserts the result at an existing grapheme index in a String */
    void insert_fmt_at_va (size_t index, char const* fmt, va_list args);

    /* Wrapper for vsnprintf that inserts the result at an existing grapheme index in a String */
    void insert_fmt_at (size_t index, uint8_t const* fmt, ...);

    /* Wrapper for vsnprintf that inserts the result at an existing grapheme index in a String */
    void insert_fmt_at (size_t index, char const* fmt, ...);

    /* Create a new copy of a String with all known graphemes converted to their lowercase equivalent */
    String to_lowercase () const;

    /* Create a new copy of a String with all known graphemes converted to their uppercase equivalent */
    String to_uppercase () const;

    // utf8proc totitle is broken
    // String to_titlecase () const;

    /* Create a new copy of a String with all known graphemes converted to their lowercase equivalent
     * (This is more aggressive than to_lowercase for hashmaps and other things where case is irrelevant) */
    String casefold () const;
  };
}
