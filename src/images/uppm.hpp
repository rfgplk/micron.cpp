#pragma once

namespace uppm
{

using size_t = __UINTMAX_TYPE__;
using i32 = __INT32_TYPE__;
using u32 = __UINT32_TYPE__;
using u16 = __UINT16_TYPE__;
using u8 = __UINT8_TYPE__;

template <typename T, typename Uxp> inline constexpr bool is_same_v = __is_same(T, Uxp);

template <typename T, typename U>
concept __same_as = is_same_v<T, U>;

template <typename T, typename U>
concept same_as = __same_as<T, U> && __same_as<U, T>;

template <typename T>
concept __string_like = requires(T t) {
  { t.c_str() } -> same_as<const char *>;
  { t.data() } -> same_as<typename T::pointer>;
  { t.size() };
  { t.begin() } -> same_as<typename T::iterator>;
  { t.end() } -> same_as<typename T::iterator>;
  { t.cbegin() } -> same_as<typename T::const_iterator>;
  { t.cend() } -> same_as<typename T::const_iterator>;
};

template <typename T, typename I>
concept __container_like = requires(T t, I i) {
  { t[i] } -> same_as<typename T::reference>;
  { t.data() } -> same_as<typename T::pointer>;
  { t.cbegin() } -> same_as<typename T::const_iterator>;
  { t.cend() } -> same_as<typename T::const_iterator>;
  { t.begin() } -> same_as<typename T::iterator>;
  { t.end() } -> same_as<typename T::iterator>;
} && !requires(T t) {
  { t.c_str() } -> same_as<const char *>;
};

template <typename T>
inline constexpr T
clamp(const T &val, const T &lo, const T &hi)
{
  return val < lo ? lo : (val > hi ? hi : val);
}

template <typename T>
inline constexpr T
pow(T base, i32 exp)
{
  T result = 1;
  while ( exp > 0 ) {
    if ( exp & 1 ) {
      result *= base;
    }
    base *= base;
    exp >>= 1;
  }
  return result;
}

inline double
pow(double base, double exp)
{
  return __builtin_pow(base, exp);
}

struct pixel {
  double r;
  double g;
  double b;
};

class writer
{
public:
  template <__string_like S, template <typename> class C>
  static void
  write(S &output, i32 width, i32 height, const C<pixel> &pixels, bool binary = true)
  {
    output.clear();

    if ( binary ) {
      write_binary(output, width, height, pixels);
    } else {
      write_ascii(output, width, height, pixels);
    }
  }

  static void
  validate(const char *header)
  {
    if ( header[0] != 'P' || (header[1] != '3' && header[1] != '6') ) {
      throw std::runtime_error("Invalid PPM signature");
    }
  }

private:
  template <__string_like S, template <typename> class C>
  static void
  write_binary(S &output, i32 width, i32 height, const C<pixel> &pixels)
  {
    output.append("P6\n");

    char dims[64];
    i32 len = 0;
    i32 w = width;
    i32 h = height;

    char width_str[32];
    char height_str[32];
    i32 width_len = 0;
    i32 height_len = 0;

    if ( w == 0 ) {
      width_str[width_len++] = '0';
    } else {
      i32 temp = w;
      i32 temp_len = 0;
      char temp_str[32];
      while ( temp > 0 ) {
        temp_str[temp_len++] = '0' + (temp % 10);
        temp /= 10;
      }
      for ( i32 i = temp_len - 1; i >= 0; --i ) {
        width_str[width_len++] = temp_str[i];
      }
    }

    if ( h == 0 ) {
      height_str[height_len++] = '0';
    } else {
      i32 temp = h;
      i32 temp_len = 0;
      char temp_str[32];
      while ( temp > 0 ) {
        temp_str[temp_len++] = '0' + (temp % 10);
        temp /= 10;
      }
      for ( i32 i = temp_len - 1; i >= 0; --i ) {
        height_str[height_len++] = temp_str[i];
      }
    }

    for ( i32 i = 0; i < width_len; ++i ) {
      dims[len++] = width_str[i];
    }
    dims[len++] = ' ';
    for ( i32 i = 0; i < height_len; ++i ) {
      dims[len++] = height_str[i];
    }
    dims[len++] = '\n';

    output.append(dims, len);
    output.append("255\n");

    const auto to_byte = [](double val) {
      const double clamped = clamp(val, 0.0, 1.0);
      return static_cast<u8>(pow(clamped, 1.0 / 2.2) * 255.0 + 0.5);
    };

    for ( i32 i = 0; i < width * height; ++i ) {
      const pixel &px = pixels[i];
      const u8 rgb[3] = { to_byte(px.r), to_byte(px.g), to_byte(px.b) };
      output.append(reinterpret_cast<const char *>(rgb), 3);
    }
  }

  template <__string_like S, template <typename> class C>
  static void
  write_ascii(S &output, i32 width, i32 height, const C<pixel> &pixels)
  {
    output.append("P3\n");

    char dims[64];
    i32 len = 0;
    i32 w = width;
    i32 h = height;

    char width_str[32];
    char height_str[32];
    i32 width_len = 0;
    i32 height_len = 0;

    if ( w == 0 ) {
      width_str[width_len++] = '0';
    } else {
      i32 temp = w;
      i32 temp_len = 0;
      char temp_str[32];
      while ( temp > 0 ) {
        temp_str[temp_len++] = '0' + (temp % 10);
        temp /= 10;
      }
      for ( i32 i = temp_len - 1; i >= 0; --i ) {
        width_str[width_len++] = temp_str[i];
      }
    }

    if ( h == 0 ) {
      height_str[height_len++] = '0';
    } else {
      i32 temp = h;
      i32 temp_len = 0;
      char temp_str[32];
      while ( temp > 0 ) {
        temp_str[temp_len++] = '0' + (temp % 10);
        temp /= 10;
      }
      for ( i32 i = temp_len - 1; i >= 0; --i ) {
        height_str[height_len++] = temp_str[i];
      }
    }

    for ( i32 i = 0; i < width_len; ++i ) {
      dims[len++] = width_str[i];
    }
    dims[len++] = ' ';
    for ( i32 i = 0; i < height_len; ++i ) {
      dims[len++] = height_str[i];
    }
    dims[len++] = '\n';

    output.append(dims, len);
    output.append("255\n");

    const auto to_byte = [](double val) {
      const double clamped = clamp(val, 0.0, 1.0);
      return static_cast<u8>(pow(clamped, 1.0 / 2.2) * 255.0 + 0.5);
    };

    char buffer[16];
    for ( i32 i = 0; i < width * height; ++i ) {
      const pixel &px = pixels[i];
      const u8 r = to_byte(px.r);
      const u8 g = to_byte(px.g);
      const u8 b = to_byte(px.b);

      i32 r_len = int_to_str(r, buffer);
      output.append(buffer, r_len);
      output.append(" ");

      i32 g_len = int_to_str(g, buffer);
      output.append(buffer, g_len);
      output.append(" ");

      i32 b_len = int_to_str(b, buffer);
      output.append(buffer, b_len);

      if ( (i + 1) % width == 0 ) {
        output.append("\n");
      } else {
        output.append(" ");
      }
    }
  }

  static i32
  int_to_str(u8 val, char *buffer)
  {
    if ( val == 0 ) {
      buffer[0] = '0';
      return 1;
    }

    i32 len = 0;
    u8 temp = val;
    while ( temp > 0 ) {
      buffer[len++] = '0' + (temp % 10);
      temp /= 10;
    }

    for ( i32 i = 0; i < len / 2; ++i ) {
      char tmp = buffer[i];
      buffer[i] = buffer[len - 1 - i];
      buffer[len - 1 - i] = tmp;
    }

    return len;
  }
};

class reader
{
public:
  template <__string_like S, template <typename> class C>
  static void
  read(const S &input, i32 &width, i32 &height, C<pixel> &pixels)
  {
    if ( input.size() < 3 ) {
      throw std::runtime_error("Input too small to be valid PPM");
    }

    writer::validate(input.data());

    const bool is_binary = (input[1] == '6');

    if ( is_binary ) {
      read_binary(input, width, height, pixels);
    } else {
      read_ascii(input, width, height, pixels);
    }
  }

  template <__string_like S>
  static void
  extract_dimensions(const S &input, i32 &width, i32 &height)
  {
    if ( input.size() < 3 ) {
      throw std::runtime_error("Input too small for PPM header");
    }

    writer::validate(input.data());

    size_t pos = 3;
    skip_whitespace_and_comments(input, pos);

    width = parse_number(input, pos);
    skip_whitespace_and_comments(input, pos);

    height = parse_number(input, pos);
  }

private:
  template <__string_like S, template <typename> class C>
  static void
  read_binary(const S &input, i32 &width, i32 &height, C<pixel> &pixels)
  {
    size_t pos = 3;
    skip_whitespace_and_comments(input, pos);

    width = parse_number(input, pos);
    skip_whitespace_and_comments(input, pos);

    height = parse_number(input, pos);
    skip_whitespace_and_comments(input, pos);

    const i32 max_val = parse_number(input, pos);
    if ( max_val != 255 ) {
      throw std::runtime_error("Only 255 max value supported");
    }

    skip_single_whitespace(input, pos);

    pixels.resize(width * height);

    const auto from_byte = [](u8 byte) {
      const double normalized = static_cast<double>(byte) / 255.0;
      return pow(normalized, 2.2);
    };

    for ( i32 i = 0; i < width * height; ++i ) {
      if ( pos + 3 > input.size() ) {
        throw std::runtime_error("Truncated pixel data");
      }

      pixel &px = pixels[i];
      px.r = from_byte(static_cast<u8>(input[pos++]));
      px.g = from_byte(static_cast<u8>(input[pos++]));
      px.b = from_byte(static_cast<u8>(input[pos++]));
    }
  }

  template <__string_like S, template <typename> class C>
  static void
  read_ascii(const S &input, i32 &width, i32 &height, C<pixel> &pixels)
  {
    size_t pos = 3;
    skip_whitespace_and_comments(input, pos);

    width = parse_number(input, pos);
    skip_whitespace_and_comments(input, pos);

    height = parse_number(input, pos);
    skip_whitespace_and_comments(input, pos);

    const i32 max_val = parse_number(input, pos);
    if ( max_val != 255 ) {
      throw std::runtime_error("Only 255 max value supported");
    }

    pixels.resize(width * height);

    const auto from_byte = [](u8 byte) {
      const double normalized = static_cast<double>(byte) / 255.0;
      return pow(normalized, 2.2);
    };

    for ( i32 i = 0; i < width * height; ++i ) {
      skip_whitespace_and_comments(input, pos);
      const i32 r = parse_number(input, pos);

      skip_whitespace_and_comments(input, pos);
      const i32 g = parse_number(input, pos);

      skip_whitespace_and_comments(input, pos);
      const i32 b = parse_number(input, pos);

      pixel &px = pixels[i];
      px.r = from_byte(static_cast<u8>(r));
      px.g = from_byte(static_cast<u8>(g));
      px.b = from_byte(static_cast<u8>(b));
    }
  }

  template <__string_like S>
  static void
  skip_whitespace_and_comments(const S &input, size_t &pos)
  {
    while ( pos < input.size() ) {
      if ( input[pos] == '#' ) {
        while ( pos < input.size() && input[pos] != '\n' ) {
          ++pos;
        }
      } else if ( input[pos] == ' ' || input[pos] == '\t' || input[pos] == '\n' || input[pos] == '\r' ) {
        ++pos;
      } else {
        break;
      }
    }
  }

  template <__string_like S>
  static void
  skip_single_whitespace(const S &input, size_t &pos)
  {
    if ( pos < input.size() && (input[pos] == ' ' || input[pos] == '\t' || input[pos] == '\n' || input[pos] == '\r') ) {
      ++pos;
    }
  }

  template <__string_like S>
  static i32
  parse_number(const S &input, size_t &pos)
  {
    i32 result = 0;
    bool found_digit = false;

    while ( pos < input.size() && input[pos] >= '0' && input[pos] <= '9' ) {
      result = result * 10 + (input[pos] - '0');
      ++pos;
      found_digit = true;
    }

    if ( !found_digit ) {
      throw std::runtime_error("Expected number");
    }

    return result;
  }
};

}
