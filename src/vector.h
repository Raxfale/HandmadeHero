//
// Vector
//

//
// Copyright (c) 2015 Peter Niekamp
//

#pragma once

#include <cmath>
#include <utility>
#include "cxxports.h"

namespace lml
{
  template<typename T, typename std::enable_if<std::is_scalar<T>::value>::type* = nullptr> T clamp(T value, T lower, T upper);
  template<typename T, typename std::enable_if<std::is_scalar<T>::value>::type* = nullptr> T lerp(T lower, T upper, T alpha);


  //|-------------------- VectorView  ---------------------------------------
  //|------------------------------------------------------------------------

  template<typename Vector, typename T, size_t... Indices>
  class VectorView
  {
    public:

      typedef T value_type;

      static constexpr size_t size() { return sizeof...(Indices); }

      Vector operator()() const { return { (*this)[Indices]... }; }

      VectorView &operator =(Vector const &v) { std::tie((*this)[Indices]...) = lml::tie(&v[0], make_index_sequence<0, size()>()); return *this; }

      template<typename V>
      VectorView &operator +=(V &&v) { *this = *this + std::forward<V>(v); return *this; }

      template<typename V>
      VectorView &operator -=(V &&v) { *this = *this - std::forward<V>(v); return *this; }

      template<typename V>
      VectorView &operator *=(V &&v) { *this = *this * std::forward<V>(v); return *this; }

      template<typename V>
      VectorView &operator /=(V &&v) { *this = *this / std::forward<V>(v); return *this; }

      constexpr T const &operator[](size_t i) const { return *((T*)this + i); }
      T &operator[](size_t i) { return *((T*)this + i); }

    protected:
      VectorView() = default;
      VectorView &operator =(VectorView const &) = default;
  };


  //|///////////////////// get //////////////////////////////////////////////
  template<size_t i, typename Vector, typename T, size_t... Indices>
  constexpr auto const &get(VectorView<Vector, T, Indices...> const &v) noexcept
  {
    return v[get<i>(std::index_sequence<Indices...>())];
  }


  //|///////////////////// operator == //////////////////////////////////////
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  bool operator ==(VectorView<Vector, T, Indices...> const &lhs, VectorView<Vector, T, Jndices...> const &rhs)
  {
    return std::foldl<std::logical_and<bool>>((lhs[Indices] == rhs[Jndices])...);
  }


  //|///////////////////// operator != //////////////////////////////////////
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  bool operator !=(VectorView<Vector, T, Indices...> const &lhs, VectorView<Vector, T, Jndices...> const &rhs)
  {
    return !(lhs == rhs);
  }


  //|///////////////////// normsqr /////////////////////////////////////////////
  /// length (norm) of the vector squared
  template<typename Vector, typename T, size_t... Indices>
  T normsqr(VectorView<Vector, T, Indices...> const &v)
  {
    return std::foldl<std::plus<T>>((v[Indices] * v[Indices])...);
  }


  //|///////////////////// norm /////////////////////////////////////////////
  /// length (norm) of the vector
  template<typename Vector, typename T, size_t... Indices>
  T norm(VectorView<Vector, T, Indices...> const &v)
  {
    using std::sqrt;

    return sqrt(normsqr(v));
  }


  //|///////////////////// scale ////////////////////////////////////////////
  /// scales a vector
  template<typename Vector, typename T, size_t... Indices>
  Vector scale(VectorView<Vector, T, Indices...> const &v, T scalar)
  {
    return { (v[Indices] * scalar)... };
  }


  //|///////////////////// normalise ////////////////////////////////////////
  /// normalise a vector to a unit vector
  template<typename Vector, typename T, size_t... Indices>
  Vector normalise(VectorView<Vector, T, Indices...> const &v)
  {
    return scale(v, 1/norm(v));
  }


  //|///////////////////// abs //////////////////////////////////////////////
  /// elementwise abs
  template<typename Vector, typename T, size_t... Indices>
  Vector abs(VectorView<Vector, T, Indices...> const &v)
  {
    using std::abs;

    return { abs(v[Indices])... };
  }


  //|///////////////////// min //////////////////////////////////////////////
  /// elementwise min
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  Vector min(VectorView<Vector, T, Indices...> const &u, VectorView<Vector, T, Jndices...> const &v)
  {
    using std::min;

    return { min(u[Indices], v[Jndices])... };
  }


  //|///////////////////// max //////////////////////////////////////////////
  /// elementwise max
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  Vector max(VectorView<Vector, T, Indices...> const &u, VectorView<Vector, T, Jndices...> const &v)
  {
    using std::max;

    return { max(u[Indices], v[Jndices])... };
  }


  //|///////////////////// clamp ////////////////////////////////////////////
  /// elementwise clamp
  template<typename Vector, typename T, size_t... Indices>
  Vector clamp(VectorView<Vector, T, Indices...> const &v, T lower, T upper)
  {
    using lml::clamp;

    return { clamp(v[Indices], lower, upper)... };
  }


  //|///////////////////// lerp /////////////////////////////////////////////
  /// elementwise lerp
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  Vector lerp(VectorView<Vector, T, Indices...> const &lower, VectorView<Vector, T, Jndices...> const &upper, T alpha)
  {
    return { lerp(lower[Indices], upper[Jndices], alpha)... };
  }


  //|///////////////////// perp /////////////////////////////////////////////
  /// perp operator
  template<typename Vector, typename T, size_t... Indices, typename std::enable_if<sizeof...(Indices) == 2>::type* = nullptr>
  Vector perp(VectorView<Vector, T, Indices...> const &v)
  {
    return { -get<1>(v), get<0>(v) };
  }


  //|///////////////////// perp /////////////////////////////////////////////
  /// perp product
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices, typename std::enable_if<sizeof...(Indices) == 2>::type* = nullptr>
  T perp(VectorView<Vector, T, Indices...> const &u, VectorView<Vector, T, Jndices...> const &v)
  {
    return get<0>(u) * get<1>(v) - get<1>(u) * get<0>(v);
  }


  //|///////////////////// dot //////////////////////////////////////////////
  /// dot product
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  T dot(VectorView<Vector, T, Indices...> const &u, VectorView<Vector, T, Jndices...> const &v)
  {
    return std::foldl<std::plus<T>>((u[Indices] * v[Jndices])...);
  }


  //|///////////////////// cross ////////////////////////////////////////////
  /// cross product
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices, typename std::enable_if<sizeof...(Indices) == 3>::type* = nullptr>
  Vector cross(VectorView<Vector, T, Indices...> const &u, VectorView<Vector, T, Jndices...> const &v)
  {
    return { get<1>(u) * get<2>(v) - get<2>(u) * get<1>(v), get<2>(u) * get<0>(v) - get<0>(u) * get<2>(v), get<0>(u) * get<1>(v) - get<1>(u) * get<0>(v) };
  }


  //|///////////////////// hada /////////////////////////////////////////////
  /// hadamard product
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  Vector hada(VectorView<Vector, T, Indices...> const &u, VectorView<Vector, T, Jndices...> const &v)
  {
    return { (u[Indices] * v[Jndices])... };
  }


  //|///////////////////// operator - ///////////////////////////////////////
  /// Vector Subtraction
  template<typename Vector, typename T, size_t... Indices>
  Vector operator -(VectorView<Vector, T, Indices...> const &v)
  {
    return scale(v, T(-1));
  }


  //|///////////////////// operator + ///////////////////////////////////////
  /// Vector Addition
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  Vector operator +(VectorView<Vector, T, Indices...> const &v1, VectorView<Vector, T, Jndices...> const &v2)
  {
    return { (v1[Indices] + v2[Jndices])... };
  }


  //|///////////////////// operator - ///////////////////////////////////////
  /// Vector Subtraction
  template<typename Vector, typename T, size_t... Indices, size_t... Jndices>
  Vector operator -(VectorView<Vector, T, Indices...> const &v1, VectorView<Vector, T, Jndices...> const &v2)
  {
    return { (v1[Indices] - v2[Jndices])... };
  }


  //|///////////////////// operator * ///////////////////////////////////////
  /// Vector multiplication by scalar
  template<typename Vector, typename T, size_t... Indices, typename S, typename std::enable_if<std::is_arithmetic<S>::value>::type* = nullptr>
  Vector operator *(S s, VectorView<Vector, T, Indices...> const &v)
  {
    return scale(v, T(s));
  }


  //|///////////////////// operator * ///////////////////////////////////////
  /// Vector multiplication by scalar
  template<typename Vector, typename T, size_t... Indices, typename S, typename std::enable_if<std::is_arithmetic<S>::value>::type* = nullptr>
  Vector operator *(VectorView<Vector, T, Indices...> const &v, S s)
  {
    return scale(v, T(s));
  }


  //|///////////////////// operator / ///////////////////////////////////////
  /// Vector division by scalar
  template<typename Vector, typename T, size_t... Indices, typename S, typename std::enable_if<std::is_arithmetic<S>::value>::type* = nullptr>
  Vector operator /(VectorView<Vector, T, Indices...> const &v, S s)
  {
    return scale(v, 1 / T(s));
  }

} // namespace lml
