#pragma once

#include "../math/sqrt.hpp"
#include "../types.hpp"

namespace micron
{
template <typename T = float>
  requires std::is_floating_point_v<T>
struct vector_2 {
  float x, y;
  vector_2() : x(0.0f), y(0.0f) {};
  vector_2(T a, T b) : x(a), y(b) {};
  vector_2(vector_2<T> &o) : x(o.x), y(o.y) {};
  vector_2(const vector_2<T> &o) : x(o.x), y(o.y) {};
  vector_2(vector_2<T> &&o) : x(o.x), y(o.y)
  {
    o.x = 0.0f;
    o.y = 0.0f;
  };
  vector_2(const std::initializer_list<T> &o)
  {
    auto it = o.begin();
    x = *it++;
    y = *it++;
  };
  vector_2<T> &
  operator=(vector_2<T> &o)
  {
    x = o.x;
    y = o.y;
    return *this;
  }
  vector_2<T> &
  operator=(const vector_2<T> &o)
  {
    x = o.x;
    y = o.y;
    return *this;
  }
  vector_2<T> &
  operator=(vector_2<T> &&o)
  {
    x = o.x;
    y = o.y;
    o.x = 0.0f;
    o.y = 0.0f;
    return *this;
  }
  vector_2<T> &&
  operator+(const vector_2<T> &o)
  {
    return vector_2(x + o.x, y + o.y);
  }
  vector_2<T> &&
  operator-(const vector_2<T> &o)
  {
    return vector_2(x - o.x, y - o.y);
  }
  vector_2<T> &
  operator+=(const vector_2<T> &o)
  {
    x += o.x;
    y += o.y;
    return *this;
  }
  vector_2<T> &
  operator-=(const vector_2<T> &o)
  {
    x -= o.x;
    y -= o.y;
    return *this;
  }
  T
  magnitude() const
  {
    return micron::fsqrt(x * x + y * y);
  }
  vector_2<T> &&
  operator*(const float scalar)
  {
    return vector_2(x * scalar, y * scalar);
  }

  vector_2<T> &&
  dot(const vector_2<T> &o)
  {
    return vector_2(x * o.x, y * o.y);
  }
};

template <typename T>
  requires std::is_floating_point_v<T>
struct vector_3 {
  float x, y, z;
  vector_3() : x(0.0f), y(0.0f), z(0.0f) {};
  vector_3(T a, T b, T c) : x(a), y(b), z(c) {};
  vector_3(vector_3<T> &o) : x(o.x), y(o.y), z(o.z) {};
  vector_3(const vector_3<T> &o) : x(o.x), y(o.y), z(o.z) {};
  vector_3(vector_3<T> &&o) : x(o.x), y(o.y), z(o.z)
  {
    o.x = 0.0f;
    o.y = 0.0f;
    o.z = 0.0f;
  };
  vector_3(const std::initializer_list<T> &o)
  {
    auto it = o.begin();
    x = *it++;
    y = *it++;
    z = *it++;
  };
  vector_3<T> &
  operator=(vector_3<T> &o)
  {
    x = o.x;
    y = o.y;
    z = o.z;
    return *this;
  }
  vector_3<T> &
  operator=(const vector_3<T> &o)
  {
    x = o.x;
    y = o.y;
    z = o.z;
    return *this;
  }
  vector_3<T> &
  operator=(vector_3<T> &&o)
  {
    x = o.x;
    y = o.y;
    z = o.z;
    o.x = 0.0f;
    o.y = 0.0f;
    o.z = 0.0f;
    return *this;
  }
  vector_3<T> &&
  operator+(const vector_3<T> &o)
  {
    return vector_3(x + o.x, y + o.y, z + o.z);
  }
  vector_3<T> &&
  operator-(const vector_3<T> &o)
  {
    return vector_3(x - o.x, y - o.y, z - o.z);
  }
  vector_3<T> &
  operator+=(const vector_3<T> &o)
  {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
  }
  vector_3<T> &
  operator-=(const vector_3<T> &o)
  {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
  }
  T
  magnitude() const
  {
    return micron::fsqrt(x * x + y * y + z * z);
  }
  vector_3<T> &&
  operator*(const float scalar)
  {
    return vector_3(x * scalar, y * scalar, z * scalar);
  }

  vector_3<T> &&
  dot(const vector_3<T> &o)
  {
    return vector_3(x * o.x, y * o.y, z * o.z);
  }
};
template <typename T>
  requires std::is_floating_point_v<T>
struct vector_4 {
  float x, y, z, w;
  vector_4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {};
  vector_4(T a, T b, T c, T d) : x(a), y(b), z(c), w(c) {};
  vector_4(vector_4<T> &o) : x(o.x), y(o.y), z(o.z), w(o.w) {};
  vector_4(const vector_4<T> &o) : x(o.x), y(o.y), z(o.z), w(o.w) {};
  vector_4(vector_4<T> &&o) : x(o.x), y(o.y), z(o.z), w(o.w)
  {
    o.x = 0.0f;
    o.y = 0.0f;
    o.z = 0.0f;
    o.w = 0.0f;
  };
  vector_4(const std::initializer_list<T> &o)
  {
    auto it = o.begin();
    x = *it++;
    y = *it++;
    z = *it++;
    w = *it++;
  };
  vector_4<T> &
  operator=(vector_4<T> &o)
  {
    x = o.x;
    y = o.y;
    z = o.z;
    w = o.z;
    return *this;
  }
  vector_4<T> &
  operator=(const vector_4<T> &o)
  {
    x = o.x;
    y = o.y;
    z = o.z;
    w = o.z;
    return *this;
  }
  vector_4<T> &
  operator=(vector_4<T> &&o)
  {
    x = o.x;
    y = o.y;
    z = o.z;
    w = o.z;
    o.x = 0.0f;
    o.y = 0.0f;
    o.z = 0.0f;
    o.w = 0.0f;
    return *this;
  }
  vector_4<T> &&
  operator+(const vector_4<T> &o)
  {
    return vector_3(x + o.x, y + o.y, z + o.z, w + o.w);
  }
  vector_4<T> &&
  operator-(const vector_4<T> &o)
  {
    return vector_3(x - o.x, y - o.y, z - o.z, w - o.w);
  }
  vector_4<T> &
  operator+=(const vector_4<T> &o)
  {
    x += o.x;
    y += o.y;
    z += o.z;
    w += o.w;
    return *this;
  }
  vector_4<T> &
  operator-=(const vector_4<T> &o)
  {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    w -= o.w;
    return *this;
  }
  T
  magnitude() const
  {
    return micron::fsqrt(x * x + y * y + z * z + w * w);
  }
  vector_4<T> &&
  operator*(const float scalar)
  {
    return vector_3(x * scalar, y * scalar, z * scalar, w * scalar);
  }

  vector_4<T> &&
  dot(const vector_4<T> &o)
  {
    return vector_3(x * o.x, y * o.y, z * o.z, w * o.w);
  }
};
typedef vector_2<float> vec2;
typedef vector_3<float> vec3;
typedef vector_4<float> vec4;

typedef vector_2<double> dvec2;
typedef vector_3<double> dvec3;
typedef vector_4<double> dvec4;
};     // namespace micron
