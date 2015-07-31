//
// Vector
//

//
// Copyright (c) 2015 Peter Niekamp
//

#pragma once

namespace lml
{

  //|-------------------- BoundView  ----------------------------------------
  //|------------------------------------------------------------------------

  template<typename Bound, typename T, size_t Stride, size_t... Indices>
  class BoundView
  {
    public:

      typedef T value_type;

      static constexpr size_t size() { return sizeof...(Indices); }
      static constexpr size_t stride() { return Stride; }

      Bound operator()() const { return { { (*this)[0][Indices]... }, { (*this)[1][Indices]... } }; }

      BoundView &operator =(Bound const &b) { size_t i = 0; (void)(int[]){ (void((*this)[0][Indices] = b[0][i]),((*this)[1][Indices] = b[1][i]),++i)... }; return *this; }

      constexpr T const *operator[](size_t j) const { return ((T*)this + j*Stride); }
      T *operator[](size_t j) { return ((T*)this + j*Stride); }

    protected:
      BoundView() = default;
      BoundView &operator =(BoundView const &) = default;
  };


  //|///////////////////// expand ///////////////////////////////////////////
  /// Expansion of two bounds (union)
  template<typename Bound, typename T, size_t IStride, size_t... Indices, size_t JStride, size_t... Jndices>
  auto expand(BoundView<Bound, T, IStride, Indices...> const &b1, BoundView<Bound, T, JStride, Jndices...> const &b2)
  {
    return Bound({ std::min(b1[0][Indices], b2[0][Jndices])... }, { std::max(b1[1][Indices], b2[1][Jndices])... });
  }


  //|///////////////////// contains /////////////////////////////////////////
  /// bound contains point
  template<typename Bound, typename T, size_t Stride, size_t... Indices, typename Point, size_t... Kndices>
  bool contains(BoundView<Bound, T, Stride, Indices...> const &bound, Point const &pt, std::index_sequence<Kndices...>)
  {
    return std::foldl<std::logical_and<bool>>((bound[0][Indices] <= get<Kndices>(pt) && get<Kndices>(pt) <= bound[1][Indices])...);
  }

  template<typename Bound, typename T, size_t Stride, size_t... Indices, typename Point>
  bool contains(BoundView<Bound, T, Stride, Indices...> const &bound, Point const &pt)
  {
    return contains(bound, pt, make_index_sequence<0, pt.size()>());
  }


  //|///////////////////// contains /////////////////////////////////////////
  /// bound contains bound
  template<typename Bound, typename T, size_t IStride, size_t... Indices, size_t JStride, size_t... Jndices>
  bool contains(BoundView<Bound, T, IStride, Indices...> const &b1, BoundView<Bound, T, JStride, Jndices...> const &b2)
  {
    return std::foldl<std::logical_and<bool>>((b2[0][Jndices] >= b1[0][Indices] && b2[1][Jndices] <= b1[1][Indices])...);
  }

} // namespace lml
