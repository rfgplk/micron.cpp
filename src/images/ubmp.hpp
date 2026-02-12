#pragma once

#include "../algorithm/algorithm.hpp"
#include "../except.hpp"
#include "../memory/cmemory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

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

struct pixel {
  double r;
  double g;
  double b;
};

struct header_t {
  u16 type{ 0x4D42 };
  u32 size;
  u16 reserved1{ 0 };
  u16 reserved2{ 0 };
  u32 offset{ 54 };
} __attribute__((packed));

struct dib_header_t {
  u32 size{ 40 };
  i32 width;
  i32 height;
  u16 planes{ 1 };
  u16 bits_per_pixel{ 24 };
  u32 compression{ 0 };
  u32 image_size{ 0 };
  i32 x_pixels_per_meter{ 2835 };
  i32 y_pixels_per_meter{ 2835 };
  u32 colors_used{ 0 };
  u32 colors_important{ 0 };
} __attribute__((packed));

struct writer {
  template <__string_like S, template <typename> class C>
  static void
  write(S &output, i32 width, i32 height, const C<pixel> &pixels)
  {
    const i32 row_size = ((width * 3 + 3) / 4) * 4;

    header_t bmp_header;
    bmp_header.size = 54 + row_size * height;

    dib_header_t dib_header;
    dib_header.width = width;
    dib_header.height = height;
    dib_header.image_size = row_size * height;

    output.resize(bmp_header.size);
    auto append = []<typename T>(S &out, T header, u64 offset) {
      u8 *start = reinterpret_cast<u8 *>(&header);
      for ( u64 i = offset; i < sizeof(header); ++i )
        out[i] = *(start + (i - offset));
    };
    append(output, bmp_header, 0);
    append(output, dib_header, sizeof(bmp_header));
    u64 itr = sizeof(bmp_header) + sizeof(dib_header);
    alignas(32) u8 row[row_size] = {};
    for ( i32 y = height - 1; y >= 0; --y ) {
      for ( i32 x = 0; x < width; ++x ) {
        const pixel &px = pixels[y * width + x];
        const auto to_byte = [](double val) {
          const double clamped = clamp(val, 0.0, 1.0);
          return static_cast<u8>(math::powerf(clamped, 1.0 / 2.2) * 255.0 + 0.5);
        };
        row[x * 3 + 0] = to_byte(px.b);
        row[x * 3 + 1] = to_byte(px.g);
        row[x * 3 + 2] = to_byte(px.r);
      }
      for ( u64 i = 0; i < row_size; i += 4 ) {
        output[itr + i] = row[i];
        output[itr + i + 1] = row[i + 1];
        output[itr + i + 2] = row[i + 2];
        output[itr + i + 3] = row[i + 3];
      }
      itr += row_size;
    }
  }

  static bool
  validate(const header_t &header, const dib_header_t &dib)
  {
    if ( header.type != 0x4D42 ) {
      return false;
    }
    if ( dib.bits_per_pixel != 24 ) {
      return false;
    }
    if ( dib.compression != 0 ) {
      return false;
    }
    return true;
  }
};

struct reader {
  template <__string_like S, template <typename> class C>
  static void
  read(const S &input, i32 &width, i32 &height, C<pixel> &pixels)
  {
    if ( input.size() < sizeof(header_t) + sizeof(dib_header_t) ) {
      exc<except::library_error>("Input too small to be valid BMP");
    }

    header_t bmp_header;
    dib_header_t dib_header;

    voidcpy(&bmp_header, input.data(), sizeof(bmp_header) / sizeof(u8));
    voidcpy(&dib_header, input.data() + sizeof(bmp_header), sizeof(dib_header) / sizeof(u8));

    writer::validate(bmp_header, dib_header);

    width = dib_header.width;
    height = dib_header.height;

    const i32 row_size = ((width * 3 + 3) / 4) * 4;
    pixels.resize(width * height);

    const u8 *data_ptr = reinterpret_cast<const u8 *>(input.data()) + bmp_header.offset;

    for ( i32 y = height - 1; y >= 0; --y ) {
      const u8 *row = data_ptr + (height - 1 - y) * row_size;
      for ( i32 x = 0; x < width; ++x ) {
        const auto from_byte = [](u8 byte) {
          const double normalized = static_cast<double>(byte) / 255.0;
          return math::powerf(normalized, 2.2);
        };
        pixel &px = pixels[y * width + x];
        px.b = from_byte(row[x * 3 + 0]);
        px.g = from_byte(row[x * 3 + 1]);
        px.r = from_byte(row[x * 3 + 2]);
      }
    }
  }

  template <__string_like S>
  static header_t
  extract_header(const S &input)
  {
    if ( input.size() < sizeof(header_t) ) {
      exc<except::library_error>("Input too small for BMP header");
    }

    header_t bmp_header;
    voidcpy(&bmp_header, input.data(), sizeof(bmp_header) / sizeof(u8));
    return bmp_header;
  }

  template <__string_like S>
  static dib_header_t
  extract_dib_header(const S &input)
  {
    if ( input.size() < sizeof(header_t) + sizeof(dib_header_t) ) {
      exc<except::library_error>("Input too small for DIB header");
    }

    dib_header_t dib_header;
    voidcpy(&dib_header, input.data() + sizeof(header_t), sizeof(dib_header) / sizeof(u8));
    return dib_header;
  }
};
};
