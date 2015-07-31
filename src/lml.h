//
// Handmade Hero - math types
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "vector.h"
#include "bound.h"

namespace lml
{

  //|-------------------- Vec2 ----------------------------------------------
  //|------------------------------------------------------------------------

  class Vec2 : public VectorView<Vec2, float, 0, 1>
  {
    public:
      Vec2() = default;
      constexpr Vec2(float x, float y);

    union
    {
      struct
      {
        float x;
        float y;
      };
    };
  };


  //|///////////////////// Vec2::Constructor //////////////////////////////////
  constexpr Vec2::Vec2(float x, float y)
    : x(x), y(y)
  {
  }


  //|-------------------- Vec3 ----------------------------------------------
  //|------------------------------------------------------------------------

  class Vec3 : public VectorView<Vec3, float, 0, 1, 2>
  {
    public:
      Vec3() = default;
      constexpr Vec3(float x, float y, float z);
      constexpr Vec3(Vec2 const &xy, float z);

    union
    {
      struct
      {
        float x;
        float y;
        float z;
      };

      Vec2 xy;
    };
  };


  //|///////////////////// Vec3::Constructor //////////////////////////////////
  constexpr Vec3::Vec3(float x, float y, float z)
    : x(x), y(y), z(z)
  {
  }


  //|///////////////////// Vec3::Constructor //////////////////////////////////
  constexpr Vec3::Vec3(Vec2 const &xy, float z)
    : Vec3(xy.x, xy.y, z)
  {
  }



  //|-------------------- Rect2 ---------------------------------------------
  //|------------------------------------------------------------------------

  class Rect2 : public BoundView<Rect2, float, 2, 0, 1>
  {
    public:
      Rect2() = default;
      Rect2(Vec2 const &min, Vec2 const &max);

      Vec2 centre() const { return (min + max)/2; }
      Vec2 halfdim() const { return (max - min)/2; }

      Vec2 min;
      Vec2 max;
  };


  //|///////////////////// Rect2::Constructor ///////////////////////////////
  inline Rect2::Rect2(Vec2 const &min, Vec2 const &max)
    : min(min), max(max)
  {
  }


  //|-------------------- Rect3 ---------------------------------------------
  //|------------------------------------------------------------------------

  class Rect3 : public BoundView<Rect3, float, 3, 0, 1, 2>
  {
    public:
      Rect3() = default;
      Rect3(Vec3 const &min, Vec3 const &max);

      Vec3 centre() const { return (min + max)/2; }
      Vec3 halfdim() const { return (max - min)/2; }

      Vec3 min;
      Vec3 max;
  };


  //|///////////////////// Rect3::Constructor ///////////////////////////////
  inline Rect3::Rect3(Vec3 const &min, Vec3 const &max)
    : min(min), max(max)
  {
  }


  /**
   * Misc Maths
  **/


  //|//////////// clamp /////////////////////////////////////////////////////
  template<typename T, typename std::enable_if<std::is_scalar<T>::value>::type* = nullptr>
  T clamp(T value, T lower, T upper)
  {
    return std::max(lower, std::min(value, upper));
  }


  //|//////////// lerp //////////////////////////////////////////////////////
  template<typename T, typename std::enable_if<std::is_scalar<T>::value>::type* = nullptr>
  T lerp(T lower, T upper, T alpha)
  {
    return (1-alpha)*lower + alpha*upper;
  }


  //|//////////// remap /////////////////////////////////////////////////////
  template<typename T, typename std::enable_if<std::is_scalar<T>::value>::type* = nullptr>
  T remap(T value1, T lower1, T upper1, T lower2, T upper2)
  {
    return lerp(lower2, upper2, (value1 - lower1)/(upper1 - lower1));
  }
}
