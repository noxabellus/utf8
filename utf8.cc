#include "utf8.hh"

extern "C" {
  uint32_t  utf8proc_toupper (uint32_t c);
  uint32_t  utf8proc_tolower (uint32_t c);
  int utf8proc_charwidth (uint32_t c);
  uint8_t* utf8proc_NFKC_Casefold (uint8_t const* str);
}

#ifdef _WIN32
  namespace Windows {
    extern "C" {
      int __declspec(dllimport) IsValidCodePage (unsigned int);
      int __declspec(dllimport) SetConsoleCP (unsigned int);
      int __declspec(dllimport) SetConsoleOutputCP (unsigned int);
    }

    static constexpr
    unsigned int UTF8_FLAG = 65001;
  }
#endif


namespace utf8 {
  extern
  void setup_console () {
    #ifdef _WIN32
      if (Windows::IsValidCodePage(Windows::UTF8_FLAG)) {
        if (!Windows::SetConsoleCP(Windows::UTF8_FLAG)) goto err;
        if (!Windows::SetConsoleOutputCP(Windows::UTF8_FLAG)) goto err;
        return;
      }

      err: {
        printf("A UTF-8 compatible terminal is required\n");
        abort();
      }
    #endif
  }

  extern
  uint8_t char_size (uint8_t const* c) {
    const bool vals[] = {
      (*c & 0b10000000) == 0b00000000,
      (*c & 0b11100000) == 0b11000000,
      (*c & 0b11110000) == 0b11100000,
      (*c & 0b11111000) == 0b11110000,
    };

    uint8_t out = 0;

    for (uint8_t i = 0; i < 4; ++i)
      vals[i] && (out = i);

    return out + 1;
  }

  extern
  uint8_t char_size (uint32_t c) {
    const bool vals[] = {
      c < 128,
      c >= 128 && c < 2048,
      c >= 2048 && c < 65536,
      c >= 65536 && c < 1114112,
    };

    uint8_t out = 0;

    for (uint8_t i = 0; i < 4; ++i)
      if (vals[i]) out = i;

    if (c >= 1114112) {
      printf("Char code %d is out of utf8 range (Must be integer 0 - 1114112)\n", c);
      abort();
    }

    return out + 1;
  }

  extern
  uint32_t to_int (uint8_t const* c) {
    uint32_t out = *c;

    switch (char_size(c)) {
      case 1: return out;
      case 2: return ((out & 31) << 6) | (c[1] & 63);
      case 3: return ((out & 15) << 12) | ((c[1] & 63) << 6) | (c[2] & 63);
      case 4: return ((out & 7) << 18) | ((c[1] & 63) << 12) | ((c[2] & 63) << 6) | (c[3] & 63);
    }

    return 0;
  }

  extern
  size_t encode (uint32_t c, uint8_t* bytes) {
    uint8_t length = char_size(c);

    if (length == 1) {
      bytes[0] = c;
    } else if (length == 2) {
      bytes[0] = 192 + (c >> 6);
      bytes[1] = 128 + (c & 63);
    } else if (length == 3) {
      bytes[0] = 224 + (c >> 12);
      bytes[1] = 128 + ((c >> 6) & 63);
      bytes[2] = 128 + (c & 63);
    } else if (length == 4) {
      bytes[0] = 240 + (c >> 18);
      bytes[1] = 128 + ((c >> 12) & 63);
      bytes[2] = 128 + ((c >> 6) & 63);
      bytes[3] = 128 + (c & 63);
    }

    return length;
  }

  extern
  size_t put_char (uint8_t const* ustr, FILE* f) {
    uint8_t adv = char_size(ustr);
    fwrite(ustr, adv, 1, f);
    return adv;
  }

  extern
  size_t put_char (uint32_t c, FILE* f) {
    uint32_t mem = 0;
    encode(c, (uint8_t*) &mem);
    return put_char((uint8_t const*) &mem, f);
  }

  extern
  size_t get_char (uint8_t* ustr, FILE* f) {
    if (!fread(ustr, 1, 1, f)) return 0;

    uint8_t size = char_size(ustr);

    if (size > 1) {
      if (!fread(ustr + 1, size - 1, 1, f)) {
        printf("Unexpected EOF: File ends in the middle of utf8 character code\n");
        abort();
      }
    }

    return size;
  }

  extern
  uint32_t get_char (FILE* f) {
    uint32_t mem = 0;
    get_char((uint8_t*) &mem, f);
    return to_int((uint8_t*) &mem);
  }

  extern
  size_t char_count (uint8_t const* ustr, size_t max_byte_length) {
    size_t i = 0;
    size_t byte_offset = 0;

    while (ustr[byte_offset] != '\0'
    && byte_offset < max_byte_length) {
      ++ i;
      byte_offset += char_size(ustr + byte_offset);
    }

    return i;
  }

  template <typename T>
  inline
  T __char_iterate (T ustr, size_t index) {
    T o = ustr;

    for (size_t i = 0; i < index; i ++) {
      if (*o == '\0') {
        printf("Out of range utf8 character index %zu for string '%s', max index is %zu\n", index, ustr, i);
        abort();
      }

      o += char_size(o);
    }

    return o;
  }

  inline extern
  uint8_t* index_offset (uint8_t* ustr, size_t index) {
    return __char_iterate(ustr, index);
  }

  inline extern
  uint8_t const* index_offset (uint8_t const* ustr, size_t index) {
    return __char_iterate(ustr, index);
  }

  inline extern
  size_t byte_offset (uint8_t const* ustr, size_t index) {
    return (size_t) index_offset(ustr, index) - (size_t) ustr;
  }

  extern
  uint32_t char_at (uint8_t const* ustr, size_t index) {
    return to_int(index_offset(ustr, index));
  }


  inline extern
  bool is_whitespace (uint32_t c) {
    return (c >= 0x0009 && c <= 0x000D)
        || c == 0x0020
        || c == 0x0085
        || c == 0x00A0
        || c == 0x1680
        || (c >= 0x2000 && c <= 0x200A)
        || c == 0x2028
        || c == 0x2029
        || c == 0x202F
        || c == 0x205F
        || c == 0x3000
        ;
  }

  extern
  bool is_whitespace (uint8_t const* c) {
    return is_whitespace(to_int(c));
  }


  extern
  size_t column_count (uint8_t const* ustr, size_t max_byte_length) {
    size_t byte_offset = 0;
    size_t columns = 0;

    while (ustr[byte_offset] != '\0' && byte_offset < max_byte_length) {
      uint32_t uchar = to_int(ustr + byte_offset);
      uint8_t byte_size = char_size(uchar);
      columns += utf8proc_charwidth(uchar);
      byte_offset += byte_size;
    }

    return columns;
  }

  extern
  size_t column_count (uint32_t c) {
    return utf8proc_charwidth(c);
  }



  StringIteratorResult StringIterator::operator * () const {
    return { index, utf8::to_int(bytes) };
  }

  StringIterator& StringIterator::operator ++ () {
    index ++;
    bytes += utf8::char_size(bytes);
    return *this;
  }

  bool StringIterator::operator != (StringIterator const& other) const {
    return index != other.index;
  }



  void String::dispose () {
    if (bytes != NULL) {
      free(bytes);
      bytes = NULL;
    }

    byte_length = 0;
    byte_capacity = 0;
  }

  uint8_t* String::release () {
    collapse_allocation();

    uint8_t* p = bytes;

    bytes = NULL;
    dispose();

    return p;
  }

  void String::move (String& source) {
    bytes = source.bytes;
    byte_length = source.byte_length;
    byte_capacity = source.byte_capacity;
    source.bytes = NULL;
    source.dispose();
  }


  String String::from_file (char const* file_name) {
    FILE* f;

    #ifdef _WIN32
      f = NULL;
      fopen_s(&f, file_name, "rb");
    #else
      f = fopen(file_name, "rb");
    #endif

    if (f == NULL) {
      printf("Error reading file \"%s\"\n", file_name);
      abort();
    }

    fseek(f, 0, SEEK_END);

    size_t length = ftell(f);

    fseek(f, 0, SEEK_SET);

    String out { length };

    fread(out.bytes, length, 1, f);

    fclose(f);

    out.byte_length = length;
    out.bytes[length] = 0;

    return out;
  }


  void String::to_file (char const* file_name) const {
    FILE* f;

    #ifdef _WIN32
      f = NULL;
      fopen_s(&f, file_name, "wb");
    #else
      f = fopen(file_name, "wb");
    #endif

    if (f == NULL) {
      printf("Error reading file \"%s\"\n", file_name);
      abort();
    }

    fwrite(bytes, byte_length, 1, f);

    fclose(f);
  }


  void String::collapse_allocation () {
    if (byte_capacity > byte_length + 1) {
      bytes = (uint8_t*) realloc(bytes, byte_length + 1);
      byte_capacity = byte_length + 1;
    }
  }

  void String::grow_allocation (size_t additional_length) {
    size_t required_capacity = byte_length + additional_length + 1;

    size_t new_capacity = byte_capacity > 0? byte_capacity : DEFAULT_CAPACITY;

    while (new_capacity < required_capacity) new_capacity *= 2;

    if (new_capacity > byte_capacity) {
      byte_capacity = new_capacity;

      if (bytes == NULL) bytes = (uint8_t*) malloc(byte_capacity);
      else bytes = (uint8_t*) realloc(bytes, byte_capacity);

      if (bytes == NULL) {
        printf("Out of memory or other null pointer error while growing String allocation\n");
        abort();
      }
    }
  }


  void String::insert (uint8_t const* str, size_t length) {
    if (length == 0) length = utf8::byte_count(str);

    grow_allocation(length);

    memcpy(bytes + byte_length, str, length);

    byte_length += length;

    bytes[byte_length] = 0;
  }

  void String::insert (char const* str, size_t length) {
    insert((uint8_t const*) str, length);
  }

  void String::insert (uint32_t c) {
    uint8_t length = utf8::char_size(c);

    grow_allocation(length);

    if (length == 1) {
      bytes[byte_length ++] = c;
    } else if (length == 2) {
      bytes[byte_length ++] = 192 + (c >> 6);
      bytes[byte_length ++] = 128 + (c & 63);
    } else if (length == 3) {
      bytes[byte_length ++] = 224 + (c >> 12);
      bytes[byte_length ++] = 128 + ((c >> 6) & 63);
      bytes[byte_length ++] = 128 + (c & 63);
    } else if (length == 4) {
      bytes[byte_length ++] = 240 + (c >> 18);
      bytes[byte_length ++] = 128 + ((c >> 12) & 63);
      bytes[byte_length ++] = 128 + ((c >> 6) & 63);
      bytes[byte_length ++] = 128 + (c & 63);
    }

    bytes[byte_length] = 0;
  }


  void String::insert_at (size_t index, uint8_t const* seg, size_t length) {
    if (index >= utf8::char_count(bytes)) return insert(seg, length);

    if (length == 0) length = utf8::byte_count(seg);

    grow_allocation(length);

    size_t offset = utf8::byte_offset(bytes, index);

    memmove(bytes + offset + length, bytes + offset, byte_length - offset);

    memcpy(bytes + offset, seg, length);

    byte_length += length;
  }

  void String::insert_at (size_t index, char const* str, size_t length) {
    insert_at(index, (uint8_t const*) str, length);
  }

  void String::insert_at (size_t index, uint32_t c) {
    if (index >= utf8::char_count(bytes)) return insert(c);

    uint8_t length = utf8::char_size(c);

    grow_allocation(length);

    size_t offset = utf8::byte_offset(bytes, index);

    memmove(bytes + offset + length, bytes + offset, byte_length - offset);

    if (length == 1) {
      bytes[offset] = c;
    } else if (length == 2) {
      bytes[offset + 0] = 192 + (c >> 6);
      bytes[offset + 1] = 128 + (c & 63);
    } else if (length == 3) {
      bytes[offset + 0] = 224 + (c >> 12);
      bytes[offset + 1] = 128 + ((c >> 6) & 63);
      bytes[offset + 2] = 128 + (c & 63);
    } else if (length == 4) {
      bytes[offset + 0] = 240 + (c >> 18);
      bytes[offset + 1] = 128 + ((c >> 12) & 63);
      bytes[offset + 2] = 128 + ((c >> 6) & 63);
      bytes[offset + 3] = 128 + (c & 63);
    }

    byte_length += length;
  }


  void String::remove (size_t index, size_t count) {
    size_t base = utf8::byte_offset(bytes, index);
    size_t end = utf8::byte_offset(bytes, index + count);

    memmove(bytes + base, bytes + end, byte_length - end);

    byte_length -= end - base;
  }


  void String::insert_fmt_va (uint8_t const* fmt, va_list args) {
    va_list args_b;

    va_copy(args_b, args);

    int length = vsnprintf(NULL, 0, (char const*) fmt, args_b);

    va_end(args_b);

    grow_allocation(length);

    vsnprintf((char*) bytes + byte_length, length + 1, (char const*) fmt, args);

    byte_length += length;
    bytes[byte_length] = 0;
  }

  void String::insert_fmt_va (char const* fmt, va_list args) {
    insert_fmt_va((uint8_t const*) fmt, args);
  }

  void String::insert_fmt (uint8_t const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    insert_fmt_va(fmt, args);
    va_end(args);
  }

  void String::insert_fmt (char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    insert_fmt_va(fmt, args);
    va_end(args);
  }


  void String::insert_fmt_at_va (size_t index, uint8_t const* fmt, va_list args) {
    if (index >= utf8::char_count(bytes)) return insert_fmt_va(fmt, args);

    va_list args_b;

    va_copy(args_b, args);

    int length = vsnprintf(NULL, 0, (char const*) fmt, args_b);

    va_end(args_b);

    grow_allocation(length + 1);

    size_t offset = utf8::byte_offset(bytes, index);

    uint8_t* ptr = bytes + offset;

    uint8_t* dest = ptr + length + 1;
    uint8_t* dest1 = dest - 1;
    size_t move_length = byte_length - offset;
    memmove(dest, ptr, move_length); // this double move really sucks but vsnprintf writes a null terminator

    vsnprintf((char*) ptr, length + 1, (char const*) fmt, args);
    memmove(dest1, dest, move_length);

    byte_length += length;
    bytes[byte_length] = 0;
  }

  void String::insert_fmt_at_va (size_t index, char const* fmt, va_list args) {
    insert_fmt_at_va(index, (uint8_t const*) fmt, args);
  }

  void String::insert_fmt_at (size_t index, uint8_t const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    insert_fmt_at_va(index, fmt, args);
    va_end(args);
  }

  void String::insert_fmt_at (size_t index, char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    insert_fmt_at_va(index, fmt, args);
    va_end(args);
  }


  String String::to_lowercase () const {
    String out { byte_length };

    for (auto [ i, c ] : *this) out.insert(utf8proc_tolower(c));

    return out;
  }

  String String::to_uppercase () const {
    String out { byte_length };

    for (auto [ i, c ] : *this) out.insert(utf8proc_toupper(c));

    return out;
  }

  // utf8proc totitle is broken
  // String String::to_titlecase () const {
  //   String out { byte_length };

  //   for (auto [ i, c ] : *this) out.insert(utf8proc_totitle(c));

  //   return out;
  // }

  String String::casefold () const {
    uint8_t* new_bytes = utf8proc_NFKC_Casefold(bytes);

    size_t new_length = utf8::byte_count(new_bytes);

    size_t new_capacity = DEFAULT_CAPACITY;
    while (new_capacity < new_length) new_capacity *= 2;

    if (new_capacity > new_length) new_bytes = (uint8_t*) realloc(new_bytes, new_capacity);

    return { new_bytes, new_length, new_capacity };
  }
}
