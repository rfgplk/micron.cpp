// matrix_tests.cpp
// Rigorous snowball test suite for micron::int8x8_t, uint8x8_t,
// int16x16_t, uint16x16_t via int_matrix_base_avx<B,C,R>

#include "../../src/math/matrix/matrices.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ------------------------------------------------------------------ //
//  Helpers                                                            //
// ------------------------------------------------------------------ //
namespace
{

// Fill an 8x8 matrix with sequential values starting at `start`
micron::int8x8_t
make_seq8(i32 start = 0)
{
  std::initializer_list<i32> lst
      = { start + 0,  start + 1,  start + 2,  start + 3,  start + 4,  start + 5,  start + 6,  start + 7,  start + 8,  start + 9,
          start + 10, start + 11, start + 12, start + 13, start + 14, start + 15, start + 16, start + 17, start + 18, start + 19,
          start + 20, start + 21, start + 22, start + 23, start + 24, start + 25, start + 26, start + 27, start + 28, start + 29,
          start + 30, start + 31, start + 32, start + 33, start + 34, start + 35, start + 36, start + 37, start + 38, start + 39,
          start + 40, start + 41, start + 42, start + 43, start + 44, start + 45, start + 46, start + 47, start + 48, start + 49,
          start + 50, start + 51, start + 52, start + 53, start + 54, start + 55, start + 56, start + 57, start + 58, start + 59,
          start + 60, start + 61, start + 62, start + 63 };
  return micron::int8x8_t(lst);
}

// Build an 8x8 identity matrix (i32)
micron::int8x8_t
identity8()
{
  micron::int8x8_t m;
  for ( u32 i = 0; i < 8; ++i )
    m[i, i] = 1;
  return m;
}

// Build a 16x16 identity matrix (i32)
micron::int16x16_t
identity16()
{
  micron::int16x16_t m;
  for ( u32 i = 0; i < 16; ++i )
    m[i, i] = 1;
  return m;
}

// Verify all elements of an 8x8 matrix equal `val`
bool
all8(const micron::int8x8_t &m, i32 val)
{
  for ( u32 r = 0; r < 8; ++r )
    for ( u32 c = 0; c < 8; ++c )
      if ( m[r, c] != val )
        return false;
  return true;
}

bool
all16(const micron::int16x16_t &m, i32 val)
{
  for ( u32 r = 0; r < 16; ++r )
    for ( u32 c = 0; c < 16; ++c )
      if ( m[r, c] != val )
        return false;
  return true;
}

bool
eq8(const micron::int8x8_t &a, const micron::int8x8_t &b)
{
  for ( u32 r = 0; r < 8; ++r )
    for ( u32 c = 0; c < 8; ++c )
      if ( a[r, c] != b[r, c] )
        return false;
  return true;
}

bool
eq16(const micron::int16x16_t &a, const micron::int16x16_t &b)
{
  for ( u32 r = 0; r < 16; ++r )
    for ( u32 c = 0; c < 16; ++c )
      if ( a[r, c] != b[r, c] )
        return false;
  return true;
}

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== MATRIX TESTS ===");

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – default construction zeroes all elements");
  {
    micron::int8x8_t m;
    require_true(all8(m, 0));
    require(m.__size, 64u);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scalar constructor fills all elements");
  {
    micron::int8x8_t m(7);
    require_true(all8(m, 7));

    micron::int8x8_t neg(-3);
    require_true(all8(neg, -3));

    micron::int8x8_t zero(0);
    require_true(all8(zero, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – initializer_list constructor preserves order");
  {
    micron::int8x8_t m = make_seq8(0);
    // row-major: element [r][c] = r*8 + c
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c )
        require(m[r, c], (i32)(r * 8 + c));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – initializer_list wrong size throws");
  {
    require_throw([&]() {
      std::initializer_list<i32> bad = { 1, 2, 3 };
      micron::int8x8_t m(bad);
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – copy construction produces independent clone");
  {
    micron::int8x8_t a = make_seq8(10);
    micron::int8x8_t b(a);
    require_true(eq8(a, b));

    b[0, 0] = 999;
    require(a[0, 0], 10);     // a must be unaffected
    require(b[0, 0], 999);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – move construction transfers content, source zeroed");
  {
    micron::int8x8_t a = make_seq8(5);
    micron::int8x8_t b(micron::move(a));

    require(b[0, 0], 5);
    require(b[7, 7], 68);     // 5 + 63
    require_true(all8(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – copy assignment");
  {
    micron::int8x8_t a = make_seq8(1);
    micron::int8x8_t b;
    b = a;
    require_true(eq8(a, b));

    b[3, 3] = -1;
    require(a[3, 3], (i32)(3 * 8 + 3 + 1));     // 28
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – move assignment transfers content, source zeroed");
  {
    micron::int8x8_t a = make_seq8(0);
    micron::int8x8_t b(100);
    b = micron::move(a);

    require(b[0, 1], 1);
    require(b[1, 0], 8);
    require_true(all8(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scalar assignment operator=");
  {
    micron::int8x8_t m = make_seq8(0);
    m = (i32)0;
    require_true(all8(m, 0));
    m = make_seq8(1);     // non-zero content
    m = (i32)(-1);
    require_true(all8(m, -1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – operator[](r,c) read and write");
  {
    micron::int8x8_t m;
    m[0, 0] = 1;
    m[0, 7] = 2;
    m[7, 0] = 3;
    m[7, 7] = 4;
    m[3, 4] = 99;

    require(m[0, 0], 1);
    require(m[0, 7], 2);
    require(m[7, 0], 3);
    require(m[7, 7], 4);
    require(m[3, 4], 99);

    // all untouched elements remain 0
    require(m[1, 1], 0);
    require(m[6, 6], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – row() returns reference to first element of row");
  {
    micron::int8x8_t m = make_seq8(0);
    // row(r) == __mat[r*C] == element [r][0]
    for ( u32 r = 0; r < 8; ++r )
      require(m.row(r), (i32)(r * 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – col() returns reference to flat element c (row 0, col c)");
  {
    micron::int8x8_t m = make_seq8(0);
    // col(c) == __mat[c] == element [0][c]
    for ( u32 c = 0; c < 8; ++c )
      require(m.col(c), (i32)c);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scalar += increments every element");
  {
    micron::int8x8_t m(10);
    m += (i32)5;
    require_true(all8(m, 15));

    m += (i32)(-3);
    require_true(all8(m, 12));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scalar -= decrements every element");
  {
    micron::int8x8_t m(20);
    m -= (i32)7;
    require_true(all8(m, 13));

    m -= (i32)(-3);
    require_true(all8(m, 16));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scalar *= scales every element");
  {
    micron::int8x8_t m(3);
    m *= (i32)4;
    require_true(all8(m, 12));

    m *= (i32)(-1);
    require_true(all8(m, -12));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scalar /= divides every element");
  {
    micron::int8x8_t m(100);
    m /= (i32)4;
    require_true(all8(m, 25));

    m /= (i32)5;
    require_true(all8(m, 5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – add_scalar returns new matrix, source unchanged");
  {
    micron::int8x8_t m(10);
    micron::int8x8_t r = m.add_scalar(5);
    require_true(all8(r, 15));
    require_true(all8(m, 10));     // unchanged
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – sub_scalar returns new matrix, source unchanged");
  {
    micron::int8x8_t m(10);
    micron::int8x8_t r = m.sub_scalar(3);
    require_true(all8(r, 7));
    require_true(all8(m, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scale returns new matrix, source unchanged");
  {
    micron::int8x8_t m(5);
    micron::int8x8_t r = m.scale(6);
    require_true(all8(r, 30));
    require_true(all8(m, 5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – div_scalar returns new matrix, source unchanged");
  {
    micron::int8x8_t m(50);
    micron::int8x8_t r = m.div_scalar(10);
    require_true(all8(r, 5));
    require_true(all8(m, 50));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – add_scalar / sub_scalar are inverses");
  {
    micron::int8x8_t m = make_seq8(0);
    micron::int8x8_t r = m.add_scalar(100).sub_scalar(100);
    require_true(eq8(r, m));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – scale / div_scalar are inverses (exact)");
  {
    micron::int8x8_t m = make_seq8(1);     // values 1..64, all divisible by 1
    micron::int8x8_t r = m.scale(7).div_scalar(7);
    require_true(eq8(r, m));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – element-wise operator+ correctness");
  {
    micron::int8x8_t a(3), b(4);
    micron::int8x8_t c = a + b;
    require_true(all8(c, 7));
    require_true(all8(a, 3));     // sources unchanged
    require_true(all8(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – element-wise operator- correctness");
  {
    micron::int8x8_t a(10), b(3);
    micron::int8x8_t c = a - b;
    require_true(all8(c, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – element-wise operator* correctness");
  {
    micron::int8x8_t a(5), b(6);
    micron::int8x8_t c = a * b;
    require_true(all8(c, 30));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – element-wise operator/ correctness");
  {
    micron::int8x8_t a(48), b(6);
    micron::int8x8_t c = a / b;
    require_true(all8(c, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – element-wise += / -= / *= / /= correctness");
  {
    micron::int8x8_t a(10), b(3);
    a += b;
    require_true(all8(a, 13));
    a -= b;
    require_true(all8(a, 10));
    a *= b;
    require_true(all8(a, 30));
    a /= b;
    require_true(all8(a, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – element-wise ops with non-uniform matrix");
  {
    micron::int8x8_t a = make_seq8(0);     // [r,c] = r*8+c
    micron::int8x8_t b = make_seq8(0);
    micron::int8x8_t c = a + b;
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 col = 0; col < 8; ++col )
        require(c[r, col], (i32)((r * 8 + col) * 2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – transpose of identity is identity");
  {
    micron::int8x8_t id = identity8();
    micron::int8x8_t tid = id.transpose();
    require_true(eq8(id, tid));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – transpose swaps rows and columns");
  {
    micron::int8x8_t m = make_seq8(0);
    micron::int8x8_t t = m.transpose();
    // original[r][c] == transposed[c][r]
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c )
        require(t[c, r], m[r, c]);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – double transpose yields original");
  {
    micron::int8x8_t m = make_seq8(1);
    require_true(eq8(m.transpose().transpose(), m));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – mul: identity * A == A");
  {
    micron::int8x8_t id = identity8();
    micron::int8x8_t a = make_seq8(0);
    micron::int8x8_t r = id.mul(a);
    require_true(eq8(r, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – mul: A * identity == A");
  {
    micron::int8x8_t id = identity8();
    micron::int8x8_t a = make_seq8(0);
    micron::int8x8_t r = a.mul(id);
    require_true(eq8(r, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – mul: zero matrix * A == zero");
  {
    micron::int8x8_t z;
    micron::int8x8_t a = make_seq8(1);
    micron::int8x8_t r = z.mul(a);
    require_true(all8(r, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – mul: known 2-row result");
  {
    // A: row0 = [1,0,0,...], row1 = [0,1,0,...]
    // B: row0 = [2,3,0,...], row1 = [4,5,0,...]
    // A*B row0 = [2,3,...], row1 = [4,5,...]
    micron::int8x8_t A, B;
    A[0, 0] = 1;
    A[1, 1] = 1;
    B[0, 0] = 2;
    B[0, 1] = 3;
    B[1, 0] = 4;
    B[1, 1] = 5;

    micron::int8x8_t R = A.mul(B);
    require(R[0, 0], 2);
    require(R[0, 1], 3);
    require(R[1, 0], 4);
    require(R[1, 1], 5);
    // other entries 0
    require(R[0, 2], 0);
    require(R[2, 0], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – mul: scalar matrix (kI) * A == k*A");
  {
    micron::int8x8_t kI;
    for ( u32 i = 0; i < 8; ++i )
      kI[i, i] = 3;     // 3*Identity
    micron::int8x8_t a = make_seq8(0);
    micron::int8x8_t r = kI.mul(a);
    micron::int8x8_t ka = a.scale(3);
    require_true(eq8(r, ka));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int8x8_t – mul: (A*B) element [r][c] matches manual dot product");
  {
    // Use a small structured case: A[i,j]=i+1, B[i,j]=j+1
    micron::int8x8_t A, B;
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c ) {
        A[r, c] = (i32)(r + 1);
        B[r, c] = (i32)(c + 1);
      }

    micron::int8x8_t R = A.mul(B);

    // R[i,j] = sum_k A[i,k]*B[k,j] = (i+1)*(j+1)*sum_k(1) over k=0..7
    // A[i,k] = i+1 (constant in k), B[k,j] = j+1 (constant in k)
    // R[i,j] = 8*(i+1)*(j+1)
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c )
        require(R[r, c], (i32)(8 * (r + 1) * (c + 1)));
  }
  end_test_case();

  // ================================================================ //
  //  uint8x8_t                                                       //
  // ================================================================ //
  test_case("uint8x8_t – default construction zeroes all elements");
  {
    micron::uint8x8_t m;
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c )
        require(m[r, c], 0u);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("uint8x8_t – scalar and arithmetic ops");
  {
    micron::uint8x8_t m(10u);
    m += 5u;
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c )
        require(m[r, c], 15u);

    m *= 2u;
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c )
        require(m[r, c], 30u);

    m /= 3u;
    for ( u32 r = 0; r < 8; ++r )
      for ( u32 c = 0; c < 8; ++c )
        require(m[r, c], 10u);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("uint8x8_t – element-wise add/sub/mul/div");
  {
    micron::uint8x8_t a(6u), b(3u);
    require((a + b)[0, 0], 9u);
    require((a - b)[0, 0], 3u);
    require((a * b)[0, 0], 18u);
    require((a / b)[0, 0], 2u);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("uint8x8_t – identity mul and transpose");
  {
    micron::uint8x8_t id;
    for ( u32 i = 0; i < 8; ++i )
      id[i, i] = 1u;

    micron::uint8x8_t a(5u);
    a[0, 0] = 99u;

    micron::uint8x8_t r = id.mul(a);
    require(r[0, 0], 99u);
    require(r[1, 1], 5u);

    micron::uint8x8_t tid = id.transpose();
    for ( u32 i = 0; i < 8; ++i )
      require(tid[i, i], 1u);
    for ( u32 r2 = 0; r2 < 8; ++r2 )
      for ( u32 c = 0; c < 8; ++c )
        if ( r2 != c )
          require(tid[r2, c], 0u);
  }
  end_test_case();

  // ================================================================ //
  //  int16x16_t                                                      //
  // ================================================================ //
  test_case("int16x16_t – default construction zeroes all elements");
  {
    micron::int16x16_t m;
    require_true(all16(m, 0));
    require(m.__size, 256u);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – scalar constructor fills all elements");
  {
    micron::int16x16_t m(99);
    require_true(all16(m, 99));

    micron::int16x16_t neg(-7);
    require_true(all16(neg, -7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – initializer_list constructor preserves order");
  {
    std::initializer_list<i32> lst
        = { 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,
            24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
            48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
            72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
            96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
            120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
            144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
            168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
            192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215,
            216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
            240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255 };
    micron::int16x16_t m(lst);
    for ( u32 r = 0; r < 16; ++r )
      for ( u32 c = 0; c < 16; ++c )
        require(m[r, c], (i32)(r * 16 + c));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – copy and move construction");
  {
    micron::int16x16_t a(42);
    a[8, 8] = -1;

    micron::int16x16_t b(a);
    require(b[8, 8], -1);

    b[0, 0] = 9999;
    require(a[0, 0], 42);     // a unchanged

    micron::int16x16_t c(micron::move(a));
    require(c[8, 8], -1);
    require_true(all16(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – scalar arithmetic ops");
  {
    micron::int16x16_t m(4);
    m += (i32)6;
    require_true(all16(m, 10));
    m -= (i32)3;
    require_true(all16(m, 7));
    m *= (i32)3;
    require_true(all16(m, 21));
    m /= (i32)7;
    require_true(all16(m, 3));
    m = (i32)0;
    require_true(all16(m, 0));     // memset: only byte-uniform values (0, -1) are safe
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – element-wise ops");
  {
    micron::int16x16_t a(7), b(3);
    require_true(all16(a + b, 10));
    require_true(all16(a - b, 4));
    require_true(all16(a * b, 21));

    micron::int16x16_t a2(9), b2(3);
    require_true(all16(a2 / b2, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – transpose swaps rows and columns");
  {
    // build diagonal-dominant: m[r][c] = r*16 + c
    std::initializer_list<i32> lst
        = { 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,
            24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
            48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
            72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
            96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
            120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
            144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
            168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
            192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215,
            216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
            240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255 };
    micron::int16x16_t m(lst);
    micron::int16x16_t t = m.transpose();
    for ( u32 r = 0; r < 16; ++r )
      for ( u32 c = 0; c < 16; ++c )
        require(t[c, r], m[r, c]);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – double transpose yields original");
  {
    micron::int16x16_t m(3);
    for ( u32 i = 0; i < 16; ++i )
      m[i, i] = i;
    require_true(eq16(m.transpose().transpose(), m));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – transpose of identity is identity");
  {
    micron::int16x16_t id = identity16();
    micron::int16x16_t tid = id.transpose();
    require_true(eq16(id, tid));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – mul: identity * A == A");
  {
    micron::int16x16_t id = identity16();
    micron::int16x16_t a(5);
    for ( u32 i = 0; i < 16; ++i )
      a[i, i] = i;
    require_true(eq16(id.mul(a), a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – mul: A * identity == A");
  {
    micron::int16x16_t id = identity16();
    micron::int16x16_t a(2);
    a[0, 15] = 77;
    require_true(eq16(a.mul(id), a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – mul: (A*B)[r][c] matches manual dot product");
  {
    // A[i,j] = i+1, B[i,j] = j+1  → R[i,j] = 16*(i+1)*(j+1)
    micron::int16x16_t A, B;
    for ( u32 r = 0; r < 16; ++r )
      for ( u32 c = 0; c < 16; ++c ) {
        A[r, c] = (i32)(r + 1);
        B[r, c] = (i32)(c + 1);
      }
    micron::int16x16_t R = A.mul(B);
    for ( u32 r = 0; r < 16; ++r )
      for ( u32 c = 0; c < 16; ++c )
        require(R[r, c], (i32)(16 * (r + 1) * (c + 1)));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("int16x16_t – mul: zero * A == zero");
  {
    micron::int16x16_t z, a(7);
    require_true(all16(z.mul(a), 0));
  }
  end_test_case();

  // ================================================================ //
  //  uint16x16_t                                                     //
  // ================================================================ //
  test_case("uint16x16_t – default construction zeroes all elements");
  {
    micron::uint16x16_t m;
    for ( u32 r = 0; r < 16; ++r )
      for ( u32 c = 0; c < 16; ++c )
        require(m[r, c], 0u);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("uint16x16_t – scalar and arithmetic ops");
  {
    micron::uint16x16_t m(8u);
    m += 4u;
    for ( u32 r = 0; r < 16; ++r )
      for ( u32 c = 0; c < 16; ++c )
        require(m[r, c], 12u);

    m *= 3u;
    for ( u32 r = 0; r < 16; ++r )
      for ( u32 c = 0; c < 16; ++c )
        require(m[r, c], 36u);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("uint16x16_t – identity mul");
  {
    micron::uint16x16_t id;
    for ( u32 i = 0; i < 16; ++i )
      id[i, i] = 1u;

    micron::uint16x16_t a(3u);
    a[0, 0] = 100u;
    micron::uint16x16_t r = id.mul(a);
    require(r[0, 0], 100u);
    require(r[1, 1], 3u);
    require(r[15, 15], 3u);
  }
  end_test_case();

  // ================================================================ //
  //  Cross-type invariants                                           //
  // ================================================================ //
  test_case("invariant: (A+B) == (B+A) element-wise (commutativity)");
  {
    micron::int8x8_t a = make_seq8(1);
    micron::int8x8_t b = make_seq8(10);
    require_true(eq8(a + b, b + a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: (A+B)-B == A element-wise");
  {
    micron::int8x8_t a = make_seq8(0);
    micron::int8x8_t b(5);
    require_true(eq8((a + b) - b, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: scale(k) == mul(kI)");
  {
    micron::int8x8_t a = make_seq8(0);
    micron::int8x8_t kI;
    for ( u32 i = 0; i < 8; ++i )
      kI[i, i] = 2;

    require_true(eq8(a.scale(2), a.mul(kI)));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: transpose(A+B) == transpose(A) + transpose(B)");
  {
    micron::int8x8_t a = make_seq8(0);
    micron::int8x8_t b = make_seq8(1);
    require_true(eq8((a + b).transpose(), a.transpose() + b.transpose()));
  }
  end_test_case();

  sb::print("=== ALL MATRIX TESTS PASSED ===");
  return 1;
}
