//
// cxx ports fill for missing cxx functions
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include <utility>
#include <string>
#include <tuple>
#include <dirent.h>

#pragma once

namespace std
{
#if 1
  template<typename T>
  struct hash
  {
    using type = typename std::enable_if<std::is_enum<T>::value, typename std::underlying_type<T>::type>::type;

    size_t operator()(T val) const
    {
      return std::hash<type>()(static_cast<type>(val));
    }
  };
#endif

#if 0
  inline void *align(std::size_t alignment, std::size_t size, void *&ptr, std::size_t &space)
  {
    auto pn = reinterpret_cast<std::size_t>(ptr);
    auto aligned = (pn + alignment - 1) & -alignment;

    space = space - (aligned - pn);

    return ptr = reinterpret_cast<void *>(aligned);
  }
#endif

#if 1
  namespace filesystem
  {
    class path
    {
      public:

        path();
        path(std::string const &str);

        path &operator=(std::string const &str);

        std::string const &string() const { return m_path; }

        operator std::string() const { return m_path; }

        std::string extention() const;

      private:

        std::string m_path;
    };

    inline path::path()
    {
    }

    inline path::path(std::string const &str)
      : m_path(str)
    {
    }

    inline path &path::operator=(std::string const &str)
    {
      m_path = str;

      return *this;
    }

    inline std::string path::extention() const
    {
      if (m_path == "." || m_path == ".." || m_path.find('.') == std::string::npos)
        return "";

      return m_path.substr(m_path.rfind('.'));
    }


    class directory_iterator
    {
      public:
        directory_iterator();
        explicit directory_iterator(std::string const &path);
        ~directory_iterator();

        directory_iterator &operator=(directory_iterator &&other);

        bool operator ==(directory_iterator const &other) const { return m_dirent == other.m_dirent; }

        bool operator !=(directory_iterator const &other) const { return m_dirent != other.m_dirent; }

        path const &operator *() const { return m_path; }
        path const *operator ->() const { return &m_path; }

        directory_iterator &operator++();

      private:

        DIR *m_dir;
        dirent *m_dirent;

        path m_path;
    };

    inline directory_iterator::directory_iterator()
    {
      m_dir = nullptr;
      m_dirent = nullptr;
    }

    inline directory_iterator::directory_iterator(std::string const &path)
      : directory_iterator()
    {
      m_dir = opendir(path.c_str());

      if (m_dir)
        m_dirent = readdir(m_dir);

      if (m_dirent)
        m_path = m_dirent->d_name;
    }

    inline directory_iterator &directory_iterator::operator=(directory_iterator &&other)
    {
      swap(m_dir, other.m_dir);
      swap(m_dirent, other.m_dirent);
      swap(m_path, other.m_path);

      return *this;
    }

    inline directory_iterator::~directory_iterator()
    {
      if (m_dir)
        closedir(m_dir);
    }

    inline directory_iterator &directory_iterator::operator++()
    {
      m_dirent = readdir(m_dir);

      if (m_dirent)
        m_path = m_dirent->d_name;

      return *this;
    }
  }
#endif

#if 1
  // placeholder for fold expressions
  struct fold_impl
  {
    template<typename Func, typename T>
    static constexpr auto foldl(T const &first)
    {
      return first;
    }

    template<typename Func, typename T>
    static constexpr auto foldl(T &&first, T &&second)
    {
      return Func()(std::forward<T>(first), std::forward<T>(second));
    }

    template<typename Func, typename T, typename... Args>
    static constexpr auto foldl(T &&first, T &&second, Args&&... args)
    {
      return foldl<Func>(Func()(first, second), std::forward<Args>(args)...);
    }
  };

  template<typename Func, typename... Args>
  auto foldl(Args&&... args)
  {
    return fold_impl::foldl<Func>(std::forward<Args>(args)...);
  }
#endif

#if 0
  template<size_t... Indices> struct index_sequence
  {
    typedef size_t value_type;

    static constexpr size_t size() noexcept { return sizeof...(Indices); }
  };
#endif
}

namespace lml
{
#if 1
  // make_index_sequence
  // these are a little different than those proposed for std
  template<size_t i, size_t j, size_t stride, typename = void>
  struct make_index_sequence_impl
  {
    template<typename>
    struct detail;

    template<size_t... Indices>
    struct detail<std::index_sequence<Indices...>>
    {
      using type = std::index_sequence<i, Indices...>;
    };

    using type = typename detail<typename make_index_sequence_impl<i+stride, j, stride>::type>::type;
  };

  template<size_t i, size_t j, size_t stride>
  struct make_index_sequence_impl<i, j, stride, typename std::enable_if<!(i < j)>::type>
  {
    using type = std::index_sequence<>;
  };

  template<size_t i, size_t j, size_t stride = 1>
  using make_index_sequence = typename make_index_sequence_impl<i, j, stride>::type;

  // get
  template<size_t i, typename IndexSequence>
  struct get_index_sequence_impl
  {
    template<size_t, typename>
    struct detail;

    template<size_t head, size_t... rest>
    struct detail<0, std::index_sequence<head, rest...>>
    {
      static constexpr size_t value = head;
    };

    template<size_t n, size_t head, size_t... rest>
    struct detail<n, std::index_sequence<head, rest...>>
    {
      static constexpr size_t value = detail<n-1, std::index_sequence<rest...>>::value;
    };

    using type = detail<i, IndexSequence>;
  };


  template<size_t i, size_t... Indices>
  size_t get(std::index_sequence<Indices...>)
  {
    return get_index_sequence_impl<i, std::index_sequence<Indices...>>::type::value;
  }

  template<typename T, size_t... Indices>
  constexpr auto tie(T *data, std::index_sequence<Indices...>)
  {
    return std::tie(data[Indices]...);
  }

#endif
}
