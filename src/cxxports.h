//
// cxx ports fill for missing cxx functions
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include <string>
#include <dirent.h>

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
  void *align(std::size_t alignment, std::size_t size, void *&ptr, std::size_t &space)
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

}
