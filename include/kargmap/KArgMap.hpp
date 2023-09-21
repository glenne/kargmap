#pragma once
/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2003, 2004, 2005, 2010, 2011, 2012 Agilent Technologies, Inc.
 */
#include <chrono>
#include <limits>
#include <string>

// ReSharper disable once CppUnusedIncludeDirective
#include <cstring> // std::memcpy
// ReSharper disable once CppUnusedIncludeDirective
#include <algorithm>
#include <cassert>
#include <cctype>
#include <complex>
#include <cstdlib> // strtoll for gcc
#include <functional>
#include <locale>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include <cinttypes> // for PRIdX macros

#if defined(__GNUC__) && !defined(__EXCEPTIONS)
// Particle Photon does not support exceptions
#define K_EXCEPTIONS_UNSUPPORTED
#endif

#if defined(__GNUC__) && !defined(__GXX_RTTI)
// spark photon compiles with -no-rtti which omits 'typeid'
#define KARG_TYPE_ID_NAME(T) "not supported"
#endif

// windows.h defines max which torks out std::numeric_limits<T>::max()
// https://social.msdn.microsoft.com/Forums/vstudio/en-US/d986a370-d856-4f9e-9f14-53f3b18ab63e/numericlimits-problem-with-windowsh?forum=vclanguage
#undef max

#if !defined(KArgMapSerializer)
#define KArgMapSerializer KArgMapSerializer
#endif

#if !defined(KARG_TYPE_ID_NAME)
#define KARG_TYPE_ID_NAME(T) typeid(T).name()
#endif
#ifdef K_EXCEPTIONS_UNSUPPORTED
#define K_NOEXCEPT
#else
#define K_NOEXCEPT noexcept
#endif

// http://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
namespace entazza {
class KArgCustomTypeBase {
public:
  std::string m_name;
  const void *m_value;

  KArgCustomTypeBase(const std::string &name, const void *value)
      : m_name(name), m_value(value) {}
};

template <typename T> class KArgCustomType : public KArgCustomTypeBase {
public:
  T m_value;

  KArgCustomType(T value) : KArgCustomTypeBase(KARG_TYPE_ID_NAME(T), &m_value) {
    m_value = value;
  }
};

using k_arg_custom_ptr = std::shared_ptr<KArgCustomTypeBase>;

class KArgVariant;
using KDuration = std::chrono::duration<int64_t, std::nano>;
using KTimestamp =
    std::chrono::time_point<std::chrono::system_clock, KDuration>;

using k_map_string_t = std::string;
using k_map_float64_t = double;
using k_arg_map_type = std::unordered_map<k_map_string_t, KArgVariant>;
using k_arg_list_type = std::vector<KArgVariant>;
using k_arg_map_ptr = std::shared_ptr<k_arg_map_type>;
using k_arg_list_ptr = std::shared_ptr<k_arg_list_type>;

namespace KArgMapInternal { // helpers
void argVariantToString(std::string &s, const KArgVariant &val);
void argListToString(std::string &s, const k_arg_list_type &val);
void argMapToString(std::string &s, const k_arg_map_type &val);
k_arg_list_ptr k_arg_list_clone(const k_arg_list_ptr from);
k_arg_map_ptr k_arg_map_clone(const k_arg_map_ptr from);

k_arg_map_ptr customArgToString(const KArgVariant &val);
KArgVariant argMapToCustomType(const std::string typeName, k_arg_map_ptr map);

/**
 * \brief A template to allow header only creation of singleton instances.  The
 * 'trick' is to declare a static variable within a function.  The linker will
 * resolve duplicate references to this static into a single instance. \tparam T
 */
template <class T> class Singleton {
public:
  static T &Instance() {
    static T instance;
    return instance;
  }

protected:
  Singleton() = delete;
  ~Singleton() = delete;

private:
  Singleton(Singleton const &) = delete;
  Singleton &operator=(Singleton const &) = delete;
};

// Usage:
// typedef Singleton<Foo> FooSingleton;  // creates static instance of Foo
// FooSingleton::Instance();  // retrieves the static instance.  Note ::
} // namespace KArgMapInternal

class KListBase { // do not reference iterators here as older compilers need to
                  // know all about KArgVariant
  friend class KArgVariant;
  friend void KArgMapInternal::argVariantToString(std::string &s,
                                                  const KArgVariant &val);
  friend class KArgMapSerializer;

protected:
  k_arg_list_ptr m_list;
  operator k_arg_list_ptr() const { return m_list; }
};

class KMapBase { // do not reference iterators here as older compilers need to
                 // know all about KArgVariant
  friend class KArgVariant;
  friend void KArgMapInternal::argVariantToString(std::string &s,
                                                  const KArgVariant &val);
  friend class KArgMapSerializer;
  friend class KArgUtility;
  friend k_arg_map_ptr
  KArgMapInternal::customArgToString(const KArgVariant &val);

protected:
  k_arg_map_ptr m_map;
  operator k_arg_map_ptr() const { return m_map; }
};

/**
 * \brief An enumeration of basic storage types in KArgVariant.
 */
enum class KArgTypes : uint8_t {
  null,      ///< null value
  boolean,   ///< boolean value
  int8,      ///< 8 bit int
  int16,     ///< 16 bit int
  int32,     ///< 32 bit int
  int64,     ///< 64 bit int
  uint8,     ///< 8 bit unsigned int
  uint16,    ///< 16 bit unsigned int
  uint32,    ///< 32 bit unsigned int
  uint64,    ///< 64 bit unsigned int
  float32,   ///< 32 bit floating point
  float64,   ///< 64 bit floating point
  cfloat32,  ///< 32 bit floating point
  cfloat64,  ///< 64 bit floating point
  string,    ///< string value
  map,       ///< map (unordered set of name/value pairs)
  list,      ///< list (ordered collection of values)
  timestamp, ///< timestamp representing date/time
  duration,  ///< A time duration
  custom,    ///< User defined
};

namespace KArgMapInternal { // helpers
template <typename T> struct is_shared_vector : std::false_type {};

template <typename T>
struct is_shared_vector<std::shared_ptr<std::vector<T>>> : std::true_type {};

template <typename T, typename Dummy = void>
struct is_k_scalar : std::false_type {};

template <typename T>
struct is_k_scalar<
    T, typename std::enable_if<std::is_arithmetic<T>::value ||
                               std::is_same<T, std::string>::value ||
                               std::is_same<T, const char *>::value ||
                               std::is_same<T, std::complex<float>>::value ||
                               std::is_same<T, std::complex<double>>::value ||
                               std::is_same<T, KDuration>::value ||
                               std::is_same<T, KTimestamp>::value>::type>
    : std::true_type {};

/// True if the type is one of the basic types supported by KArgMap
template <typename T, typename Dummy = void>
struct is_k_base_type : std::false_type {};

template <> struct is_k_base_type<KArgVariant> : std::false_type {};

template <typename T>
struct is_k_base_type<
    T, typename std::enable_if<std::is_arithmetic<T>::value ||
                               std::is_same<T, std::complex<float>>::value ||
                               std::is_same<T, std::complex<double>>::value ||
                               std::is_same<T, std::string>::value ||
                               std::is_same<T, const char *>::value ||
                               std::is_base_of<KMapBase, T>::value ||
                               std::is_base_of<KListBase, T>::value ||
                               std::is_base_of<KArgCustomTypeBase, T>::value ||
                               std::is_same<T, k_arg_custom_ptr>::value ||
                               std::is_same<T, k_arg_list_ptr>::value ||
                               std::is_same<T, k_arg_map_ptr>::value ||
                               std::is_same<T, KDuration>::value ||
                               std::is_same<T, KTimestamp>::value>::type>
    : std::true_type {};

/// True if the type is a shared_ptr<std::vector<T>> where T is one of the
/// KArgMap base types
template <typename T, typename Dummy = void>
struct is_k_shared_vector_type : std::false_type {};

template <typename T>
struct is_k_shared_vector_type<
    std::shared_ptr<std::vector<T>>,
    typename std::enable_if<is_k_base_type<T>::value>::type> : std::true_type {
};

/// True if T is stored as a shard_ptr
template <typename T, typename Dummy = void>
struct is_k_shared_ptr_type : std::false_type {};

template <typename T>
struct is_k_shared_ptr_type<
    std::shared_ptr<T>, typename std::enable_if<is_k_base_type<T>::value>::type>
    : std::true_type { // TODO: further restrict to only list/map/custom base
                       // type
};

/// True if T is one of the types that can be returned from a get or used with
/// set
template <typename T, typename Dummy = void>
struct is_k_type : std::false_type {};

template <typename T>
struct is_k_type<
    T, typename std::enable_if<is_k_base_type<T>::value ||
                               is_k_shared_ptr_type<T>::value ||
                               is_k_shared_vector_type<T>::value>::type>
    : std::true_type {};

template <typename T, typename Dummy = void> struct k_type_info;

template <typename T> struct k_base_scalar_storage {
  typedef T type;
  typedef T storage_type;
  static const bool value = true;
  static const bool is_vector = false;
  static inline const std::string to_string(T val) {
    char buf[30];
    snprintf(buf, sizeof(buf) - 1, KArgMapInternal::k_type_info<T>::format,
             val);
    return std::string(buf);
  };
};

template <typename T> struct k_base_vector_storage {
  typedef std::shared_ptr<std::vector<T>> type;
  typedef std::shared_ptr<std::vector<T>> storage_type;
  static const bool value = true;
  static const bool is_vector = true;
};

template <typename T> inline const std::string to_string(const T val) {
  return KArgMapInternal::k_type_info<T>::to_string(val);
}

template <> struct k_type_info<bool> : k_base_scalar_storage<bool> {
  static const KArgTypes type_code = KArgTypes::boolean;
  constexpr static const char *format = nullptr;
};

template <> struct k_type_info<int8_t> : k_base_scalar_storage<int8_t> {
  static const KArgTypes type_code = KArgTypes::int8;
  constexpr static const char *format = "%" PRId8;
};

template <> struct k_type_info<int16_t> : k_base_scalar_storage<int16_t> {
  static const KArgTypes type_code = KArgTypes::int16;
  constexpr static const char *format = "%" PRId16;
};

template <> struct k_type_info<int32_t> : k_base_scalar_storage<int32_t> {
  static const KArgTypes type_code = KArgTypes::int32;
  constexpr static const char *format = "%" PRId32;
};

template <> struct k_type_info<int64_t> : k_base_scalar_storage<int64_t> {
  static const KArgTypes type_code = KArgTypes::int64;
  constexpr static const char *format = "%" PRId64;
};

template <> struct k_type_info<uint8_t> : k_base_scalar_storage<uint8_t> {
  static const KArgTypes type_code = KArgTypes::uint8;
  constexpr static const char *format = "%" PRIu8;
};

template <> struct k_type_info<uint16_t> : k_base_scalar_storage<uint16_t> {
  static const KArgTypes type_code = KArgTypes::uint16;
  constexpr static const char *format = "%" PRIu16;
};

template <> struct k_type_info<uint32_t> : k_base_scalar_storage<uint32_t> {
  static const KArgTypes type_code = KArgTypes::uint32;
  constexpr static const char *format = "%" PRIu32;
};

template <> struct k_type_info<uint64_t> : k_base_scalar_storage<uint64_t> {
  static const KArgTypes type_code = KArgTypes::uint64;
  constexpr static const char *format = "%" PRIu64;
};

template <> struct k_type_info<float> : k_base_scalar_storage<float> {
  static const KArgTypes type_code = KArgTypes::float32;
  constexpr static const char *format = "%.8g";
};

template <> struct k_type_info<double> : k_base_scalar_storage<double> {
  static const KArgTypes type_code = KArgTypes::float64;
  constexpr static const char *format = "%.15g";
};

template <>
struct k_type_info<std::complex<float>>
    : k_base_scalar_storage<std::complex<float>> {
  static const KArgTypes type_code = KArgTypes::cfloat32;
  constexpr static const char *format = "(%.8g,%.8g)";
  static inline const std::string to_string(std::complex<float> val) {
    char buf[64];
    snprintf(buf, sizeof(buf) - 1, format, val.real(), val.imag());
    return std::string(buf);
  };
};

template <>
struct k_type_info<std::complex<double>>
    : k_base_scalar_storage<std::complex<double>> {
  static const KArgTypes type_code = KArgTypes::cfloat64;
  constexpr static const char *format = "(%.15g,%.15g)";
  static inline const std::string to_string(const std::complex<double> val) {
    char buf[80];
    snprintf(buf, sizeof(buf) - 1, format, val.real(), val.imag());
    return std::string(buf);
  };
};

template <> struct k_type_info<KDuration> : k_base_scalar_storage<KDuration> {
  static const KArgTypes type_code = KArgTypes::duration;
  constexpr static const char *format = nullptr;
  static inline const std::string to_string(const KDuration val) {
    std::chrono::duration<double, std::nano> ts_ns = val;
    double secs = ts_ns.count() / 1e9;
    char buf[30];
    snprintf(buf, sizeof(buf) - 1, k_type_info<double>::format, secs);
    return std::string(buf);
  }
};

template <> struct k_type_info<KTimestamp> : k_base_scalar_storage<KTimestamp> {
  static const KArgTypes type_code = KArgTypes::timestamp;
  constexpr static const char *format = nullptr;
  static inline const std::string to_string(const KTimestamp val) {
    return k_type_info<KDuration>::to_string(val.time_since_epoch());
  }
};

template <>
struct k_type_info<std::string> : k_base_scalar_storage<std::string> {
  static const KArgTypes type_code = KArgTypes::string;
  constexpr static const char *format = "%s";
  static inline const std::string to_string(const std::string s) { return s; }
};

template <>
struct k_type_info<const char *> : k_base_scalar_storage<std::string> {
  static const KArgTypes type_code = KArgTypes::string;
  constexpr static const char *format = "%s";
  static inline const std::string to_string(const std::string s) { return s; }
};

template <>
struct k_type_info<k_arg_map_ptr> : k_base_scalar_storage<k_arg_map_ptr> {
  static const KArgTypes type_code = KArgTypes::map;
  constexpr static const char *format = nullptr;
};

template <>
struct k_type_info<k_arg_list_ptr> : k_base_scalar_storage<k_arg_list_ptr> {
  static const KArgTypes type_code = KArgTypes::list;
  constexpr static const char *format = nullptr;
};

template <typename T>
struct k_type_info<
    T, typename std::enable_if<std::is_base_of<KMapBase, T>::value>::type>
    : k_base_scalar_storage<k_arg_map_ptr> {
  static const KArgTypes type_code = KArgTypes::map;
  constexpr static const char *format = nullptr;
};

template <typename T>
struct k_type_info<
    T, typename std::enable_if<std::is_base_of<KListBase, T>::value>::type>
    : k_base_scalar_storage<k_arg_list_ptr> {
  static const KArgTypes type_code = KArgTypes::list;
  constexpr static const char *format = nullptr;
};

template <typename T>
struct k_type_info<
    T, typename std::enable_if<std::is_base_of<KArgCustomTypeBase, T>::value ||
                               std::is_same<T, k_arg_custom_ptr>::value>::type>
    : k_base_scalar_storage<std::shared_ptr<T>> {
  static const KArgTypes type_code = KArgTypes::custom;
  constexpr static const char *format = nullptr;
};

template <typename T>
struct k_type_info<std::shared_ptr<T>, typename std::enable_if<std::is_base_of<
                                           KArgCustomTypeBase, T>::value>::type>
    : k_base_scalar_storage<k_arg_custom_ptr> {
  static const KArgTypes type_code = KArgTypes::custom;
  constexpr static const char *format = nullptr;
};

template <typename T>
struct k_type_info<std::shared_ptr<std::vector<T>>,
                   typename std::enable_if<is_k_base_type<T>::value>::type>
    : k_base_vector_storage<T> {
  static const KArgTypes type_code = k_type_info<T>::type_code;
  constexpr static const char *format = k_type_info<T>::format;
};

template <typename T>
struct k_type_info<
    T, typename std::enable_if<std::is_same<T, unsigned long>::value ||
                               std::is_same<T, unsigned int>::value>::type> {
  static_assert(
      sizeof(T) == -1,
      "Use of unsupported KArgMap type unsigned long or unsigned int");
};

} // namespace KArgMapInternal

/**
 * \brief A class to contain a variant stored in a KArgMap or KArgList.  The
 * class contains meta data for the type such as it's fundamental type (e.g.
 * int32_t) and if it is a std::vector or std::complex type.
 */
class KArgVariant {
  friend class KArgMapSerializer;

public:
  /**
   * \brief A union to hold scalar values or shared_ptr values for object types.
   * Most of these fields are not directly referenced but can be handy when
   * debugging.
   */
  union argvariant_value {
    uint8_t ptr[16];
    k_arg_map_ptr map;
    k_arg_list_ptr list;
    k_arg_custom_ptr custom;
    k_map_string_t string;
    KTimestamp timestamp;
    KDuration duration;
    bool boolean;
    int8_t int8;
    int16_t int16;
    int32_t int32;
    int64_t int64;
    uint8_t uint8;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;
    float float32;
    std::complex<float> cfloat32;
    std::complex<double> cfloat64;
    k_map_float64_t float64;

    /// default constructor (for null values)
    argvariant_value() {}

    ~argvariant_value() {}
  };

  /// the type of the current element
  KArgTypes m_type = KArgTypes::null;

  /// the value of the current element
  argvariant_value m_value;

  /// True if the value is a std::vector
  bool m_vector = false;

public:
  ~KArgVariant() { reset(); }

  KArgVariant(const KArgVariant &ref) {
    // Because m_value has non-trivial elements (std::shared_ptr) we cannot use
    // a plain assignment
    //              m_value = ref.m_value;

    // First clean up any allocated objects we're replacing
    reset();
    if (ref.m_type != KArgTypes::null && ref.m_vector) {
      // Vector types are stored as a shared_ptr created with inplace new.
      // Cast it into a shared_ptr<std::vector> and new inplace into dest.
      auto vec = *reinterpret_cast<std::shared_ptr<std::vector<int8_t>> *>(
          &const_cast<KArgVariant &>(ref).m_value);
      new (&m_value) std::shared_ptr<std::vector<int8_t>>{vec};
    } else {
      switch (ref.m_type) {
      case KArgTypes::map: {
        // http://stackoverflow.com/questions/24713833/using-a-union-with-unique-ptr
        new (&m_value) k_arg_map_ptr{ref.m_value.map};
        break;
      }

      case KArgTypes::list: {
        new (&m_value) k_arg_list_ptr{ref.m_value.list};
        break;
      }

#ifndef K_CUSTOM_TYPES_UNSUPPORTED
      case KArgTypes::custom: {
        new (&m_value) k_arg_custom_ptr{ref.m_value.custom};
        break;
      }
#endif
      case KArgTypes::string: {
        new (&m_value) std::string(ref.m_value.string);
        break;
      }

      default:
        std::memcpy(&m_value, &ref.m_value, sizeof(ref.m_value));
        break;
      }
    }
    m_type = ref.m_type;
    m_vector = ref.m_vector;
  }

  bool isVector() const { return m_vector; }
  bool isMap() const { return m_type == KArgTypes::map; }
  bool isList() const { return m_type == KArgTypes::list; }
  bool isScalar() const { return !isVector() && !isMap() && !isList(); }
  bool isCustom() const { return m_type == KArgTypes::custom; }

private:
  template <typename T> size_t getVecSize() const {
    const auto &vec =
        *reinterpret_cast<const std::shared_ptr<std::vector<T>> *>(
            &m_value.ptr);
    return vec->size();
  }

public:
  size_t size() const {
    if (isVector()) {
      switch (m_type) {
      case KArgTypes::boolean:
        return getVecSize<bool>();
      case KArgTypes::int8:
        return getVecSize<int8_t>();
      case KArgTypes::int16:
        return getVecSize<int16_t>();
      case KArgTypes::int32:
        return getVecSize<int32_t>();
      case KArgTypes::int64:
        return getVecSize<int64_t>();
      case KArgTypes::uint8:
        return getVecSize<uint8_t>();
      case KArgTypes::uint16:
        return getVecSize<uint16_t>();
      case KArgTypes::uint32:
        return getVecSize<uint32_t>();
      case KArgTypes::uint64:
        return getVecSize<uint64_t>();
      case KArgTypes::float32:
        return getVecSize<float>();
      case KArgTypes::float64:
        return getVecSize<double>();
      case KArgTypes::cfloat32:
        return getVecSize<std::complex<float>>();
      case KArgTypes::cfloat64:
        return getVecSize<std::complex<double>>();
      case KArgTypes::timestamp:
        return getVecSize<KTimestamp>();
      case KArgTypes::duration:
        return getVecSize<KDuration>();
      case KArgTypes::string:
        return getVecSize<std::string>();
      default:
        return 0;
      }
    } else if (isList()) {
      return m_value.list->size();
    }
    return 0;
  }
  KArgTypes getType() const { return m_type; }

  KArgVariant &operator=(KArgVariant other) K_NOEXCEPT {
    // Strings are not blittable on all platforms.  Detect here.
    if ((m_vector || m_type != KArgTypes::string) &&
        (other.m_vector || other.m_type != KArgTypes::string)) {
      // * blittable types
      std::swap(m_type, other.m_type);
      std::swap(m_vector, other.m_vector);
      uint8_t temp[sizeof(m_value)];
      std::memcpy(temp, &m_value, sizeof(m_value));
      std::memcpy(&m_value, &other.m_value, sizeof(m_value));
      std::memcpy(&other.m_value, temp, sizeof(m_value));
      return *this;
    }

    // This is ugly.  consider just using a pool with custom string we know is
    // blittable At this point we know one of the items is a plain string so
    // special handling needed
    if (m_type == KArgTypes::string && other.m_type == KArgTypes::string) {
      // Both are strings, use swap
      std::swap(m_value.string, other.m_value.string);
      return *this;
    } else if (!other.m_vector && other.m_type == KArgTypes::string) {
      std::string s = std::move(other.m_value.string);
      std::memcpy(&other.m_value, &m_value, sizeof(m_value));
      new (&m_value) std::string(s);
    } else if (!m_vector && m_type == KArgTypes::string) {
      std::string s = std::move(m_value.string);
      std::memcpy(&m_value, &other.m_value, sizeof(m_value));
      new (&other.m_value) std::string(s);
    }
    std::swap(m_type, other.m_type);
    std::swap(m_vector, other.m_vector);

    return *this;
  }

  template <typename T>
  typename std::enable_if<KArgMapInternal::k_type_info<T>::value,
                          KArgVariant &>::type
  operator=(T value) K_NOEXCEPT {
    reset();
    m_type = KArgMapInternal::k_type_info<T>::type_code;
    m_vector = KArgMapInternal::k_type_info<T>::is_vector;
    new (&m_value)
        typename KArgMapInternal::k_type_info<T>::storage_type(value);
    return *this;
  }

  KArgVariant() : m_type(KArgTypes::null), m_vector(false) {}

  template <typename T,
            typename std::enable_if<KArgMapInternal::is_k_type<T>::value>::type
                * = nullptr>
  KArgVariant(T value)
      : m_type(KArgMapInternal::k_type_info<T>::type_code),
        m_vector(KArgMapInternal::k_type_info<T>::is_vector) {
    new (&m_value)
        typename KArgMapInternal::k_type_info<T>::storage_type(value);
  }

  template <typename T,
            typename std::enable_if<KArgMapInternal::is_k_type<T>::value>::type
                * = nullptr>
  KArgVariant(std::vector<T> &&val)
      : m_type(KArgMapInternal::k_type_info<T>::type_code), m_vector(true) {
    auto v = new std::vector<T>(std::move(val));
    new (&m_value) std::shared_ptr<std::vector<T>>{v};
  }

  template <typename T,
            typename std::enable_if<KArgMapInternal::is_k_type<T>::value>::type
                * = nullptr>
  KArgVariant(std::vector<T> &val)
      : m_type(KArgMapInternal::k_type_info<T>::type_code), m_vector(true) {
    static_assert(
        sizeof(T) != sizeof(T),
        "to set a std::vector, wrap with std::move or std::shared_ptr.");
  }

#if 0
            template <typename T>
            KArgVariant(std::shared_ptr<KArgCustomType<T>>&& val)
                : m_type(KArgTypes::custom), m_value(val), m_vector(false)
            {
            }

            template <typename T>
            KArgVariant(KArgCustomType<T> val)
                : m_type(KArgTypes::custom), m_value(std::make_shared<KArgCustomType<T>>(val)), m_vector(false)
            {
            }
#endif
  //===========================================================
  template <class T, typename std::enable_if<
                         std::is_floating_point<T>::value>::type * = nullptr>
  T get_from_string(T defaultValue) const {
    auto &s = m_value.string;
    char *end;
    auto value = std::strtod(s.c_str(), &end);
    if (end - s.c_str() != int(s.length())) {
      return defaultValue;
    }
    return convert_number_with_default(value, defaultValue);
  }

  static inline bool kargmap_strieq(const char *s1, const char *s2) {
    for (; tolower(*s1) == tolower(*s2); s1++, s2++) {
      if (*s1 == '\0')
        return true;
    }
    return false;
  }

  template <class T, typename std::enable_if<std::is_integral<T>::value>::type
                         * = nullptr>
  T get_from_string(T defaultValue) const {
    // use strtoll and strtoull instead of std::stoll and std::stoull
    // because they do not throw exceptions
    auto &s = m_value.string;
    if (s.length() == 0)
      return defaultValue;
    if (s[0] == '-') {
      char *end;
      auto value = ::strtoll(s.c_str(), &end, 10);
      if (end - s.c_str() != int(s.length())) {
        // Try coverting from to double, then to T
        auto dvalue = get_from_string<double>(double(defaultValue));
        return convert_number_with_default(dvalue, defaultValue);
      }
      return convert_number_with_default(value, defaultValue);
    } else {
      char *end;
      auto str = s.c_str();
      if (*str != 0) {
        // Do a quick check to see if the string is a boolean string such as
        // 'true'.
        auto ch = tolower(*str);
        if (ch == 'f' || ch == 't' || ch == 'y' || ch == 'n') {
          if (kargmap_strieq(str, "false") || kargmap_strieq(str, "no")) {
            return convert_number_with_default(0, defaultValue);
          } else if (kargmap_strieq(str, "true") ||
                     kargmap_strieq(str, "yes")) {
            return convert_number_with_default(1, defaultValue);
          }
        }
      }

      auto value = ::strtoull(s.c_str(), &end, 10);
      if (end - s.c_str() != int(s.length())) {
        // Try coverting from to double, then to T
        auto dvalue = get_from_string<double>(double(defaultValue));
        return convert_number_with_default(dvalue, defaultValue);
      }
      return convert_number_with_default(value, defaultValue);
    }
  }

  template <class T, typename std::enable_if<
                         std::is_same<T, KTimestamp>::value ||
                         std::is_same<T, KDuration>::value>::type * = nullptr>
  T get_from_string(T defaultValue) const {
    auto secs = get_from_string<double>(-std::numeric_limits<double>::max());
    if (secs == -std::numeric_limits<double>::max()) {
      return defaultValue;
    }
    return T(std::chrono::nanoseconds(int64_t(secs * 1e9)));
  }

  template <typename T, typename std::enable_if<
                            (std::is_arithmetic<T>::value)>::type * = nullptr>
  T get(T defaultValue) {
    switch (m_type) {
    case KArgTypes::boolean:
      return convert_number<bool>(defaultValue);
    case KArgTypes::int8:
      return convert_number<int8_t>(defaultValue);
    case KArgTypes::int16:
      return convert_number<int16_t>(defaultValue);
    case KArgTypes::int32:
      return convert_number<int32_t>(defaultValue);
    case KArgTypes::int64:
      return convert_number<int64_t>(defaultValue);
    case KArgTypes::uint8:
      return convert_number<uint8_t>(defaultValue);
    case KArgTypes::uint16:
      return convert_number<uint16_t>(defaultValue);
    case KArgTypes::uint32:
      return convert_number<uint32_t>(defaultValue);
    case KArgTypes::uint64:
      return convert_number<uint64_t>(defaultValue);
    case KArgTypes::float32:
      return convert_number<float>(defaultValue);
    case KArgTypes::float64:
      return convert_number<double>(defaultValue);
    case KArgTypes::timestamp: {
      // Convert to double seconds since epoch and then convert to requested
      // type
      std::chrono::duration<double, std::nano> ts_ns =
          m_value.timestamp.time_since_epoch();
      auto utcSeconds = ts_ns.count() / 1e9;
      KArgVariant secs(utcSeconds);
      return secs.get<T>(defaultValue);
    }
    case KArgTypes::duration: {
      std::chrono::duration<double, std::nano> duration_ns = m_value.duration;
      auto utcSeconds = double(duration_ns.count() / 1e9);
      KArgVariant secs(utcSeconds);
      return secs.get<T>(defaultValue);
    }
    case KArgTypes::string: {
      return get_from_string(defaultValue);
    }
    default:
      return defaultValue;
    }
  }

  template <typename T,
            typename std::enable_if<(
                std::is_same<T, std::complex<float>>::value)>::type * = nullptr>
  T get(T defaultValue) {
    if (m_type == KArgTypes::cfloat32) {
      return m_value.cfloat32;
    }
    return defaultValue;
  }

  template <typename T, typename std::enable_if<(
                            std::is_same<T, std::complex<double>>::value)>::type
                            * = nullptr>
  T get(T defaultValue) {
    if (m_type == KArgTypes::cfloat64) {
      return m_value.cfloat64;
    }
    return defaultValue;
  }

  template <typename T,
            typename std::enable_if<(std::is_same<T, KTimestamp>::value)>::type
                * = nullptr>
  T get(T defaultValue) {
    if (m_type == KArgTypes::timestamp) {
      return m_value.timestamp;
    }
    double secs = get<double>(-std::numeric_limits<double>::max());
    if (secs == -std::numeric_limits<double>::max()) {
      return defaultValue;
    }
    return KTimestamp(std::chrono::nanoseconds(int64_t(secs * 1e9)));
  }

  template <typename T,
            typename std::enable_if<(std::is_same<T, KDuration>::value)>::type
                * = nullptr>
  T get(T defaultValue) {
    if (m_type == KArgTypes::duration) {
      return m_value.duration;
    }
    double secs = get<double>(-std::numeric_limits<double>::max());
    if (secs == -std::numeric_limits<double>::max()) {
      return defaultValue;
    }
    return KDuration(std::chrono::nanoseconds(int64_t(secs * 1e9)));
  }

  /**
   * \brief Get a typed value from the KArgMap
   * \param defaultValue The value to return if the key is not present.
   * \return
   */
  std::string get(const std::string &defaultValue) const {
    if (m_type == KArgTypes::string) {
      return std::string(m_value.string);
    }
    if (m_vector || m_type == KArgTypes::map || m_type == KArgTypes::list) {
      return defaultValue;
    }
    return scalar_to_string();
  }

  std::string get(const char *defaultValue) const {
    if (m_type == KArgTypes::string) {
      return std::string(m_value.string);
    }
    if (m_vector || m_type == KArgTypes::map || m_type == KArgTypes::list) {
      return std::string(defaultValue);
    }
    return scalar_to_string();
  }

  //    /**
  //    * \brief Get a typed value from the KArgMap
  //    * \param defaultValue The value to return if the key is not present.
  //    * \return
  //    */
  //    KArgMap get(KArgMap& defaultValue)
  //    {
  //        if (m_type == KArgTypes::map)
  //        {
  //            return m_value.map;
  //        }
  //        return defaultValue;
  //    }

  /**
   * \brief Get a typed value from the KArgMap
   * \param defaultValue The value to return if the key is not present.
   * \return
   */
  k_arg_map_ptr get(k_arg_map_ptr defaultValue) {
    if (m_type == KArgTypes::map) {
      return *reinterpret_cast<k_arg_map_ptr *>(&m_value);
    }
    return defaultValue;
  }

  /**
   * \brief Get a typed value from the KArgList
   * \param defaultValue The value to return if the key is not present.
   * \return
   */
  k_arg_list_ptr get(k_arg_list_ptr defaultValue) {
    if (m_type == KArgTypes::list) {
      return m_value.list;
    }
    return defaultValue;
  }

  /**
   * \brief Get a typed value from the KArgList
   * \param defaultValue The value to return if the key is not present.
   * \return
   */
  template <typename T>
  std::shared_ptr<T> get(std::shared_ptr<T> defaultValue) {
    if (m_vector) {
      auto vec = *reinterpret_cast<std::shared_ptr<T> *>(&m_value);

      auto tc = KArgMapInternal::k_type_info<typename T::value_type>::type_code;
      if (tc != m_type) {
        // std::cerr << "Cannot convert requested std::vector types.\n";
        return defaultValue;
      }
      return vec;
    }

    if (m_type == KArgTypes::list || m_type == KArgTypes::map) {
      auto vec = *reinterpret_cast<std::shared_ptr<T> *>(&m_value);
      return vec;
    }

    return defaultValue;
  }

  template <typename T> std::string as_string() const {
    char buf[30];
    T val = as<T>();
    snprintf(buf, sizeof(buf) - 1, KArgMapInternal::k_type_info<T>::format,
             val);
    return std::string(buf);
  }

#ifndef K_CUSTOM_TYPES_UNSUPPORTED
  template <typename T> T getCustomType(T defaultValue = T()) {
    if (m_type == KArgTypes::map) {
      // Is there a converter registered?
      KArgVariant v = KArgMapInternal::argMapToCustomType(KARG_TYPE_ID_NAME(T),
                                                          m_value.map);
      if (v.m_type != KArgTypes::custom)
        return defaultValue;
      auto ptr =
          *reinterpret_cast<std::shared_ptr<KArgCustomType<T>> *>(&v.m_value);
      return ptr->m_value;
    }
    if (m_type != KArgTypes::custom)
      return defaultValue;
    auto ptr =
        *reinterpret_cast<std::shared_ptr<KArgCustomType<T>> *>(&m_value);
    if (ptr->m_name != KARG_TYPE_ID_NAME(T))
      return defaultValue;
    return ptr->m_value;
  }
#endif

  std::string scalar_to_string() const {
    switch (m_type) {
    case KArgTypes::int8:
      return KArgMapInternal::to_string(m_value.int8);
    case KArgTypes::int16:
      return KArgMapInternal::to_string(m_value.int16);
    case KArgTypes::int32:
      return KArgMapInternal::to_string(m_value.int32);
    case KArgTypes::int64:
      return KArgMapInternal::to_string(m_value.int64);
    case KArgTypes::uint8:
      return KArgMapInternal::to_string(m_value.uint8);
    case KArgTypes::uint16:
      return KArgMapInternal::to_string(m_value.uint16);
    case KArgTypes::uint32:
      return KArgMapInternal::to_string(m_value.uint32);
    case KArgTypes::uint64:
      return KArgMapInternal::to_string(m_value.uint64);
    case KArgTypes::float32:
      return KArgMapInternal::to_string(m_value.float32);
    case KArgTypes::float64:
      return KArgMapInternal::to_string(m_value.float64);
    case KArgTypes::cfloat32:
      return KArgMapInternal::to_string(m_value.cfloat32);
    case KArgTypes::cfloat64:
      return KArgMapInternal::to_string(m_value.cfloat64);
    case KArgTypes::timestamp:
      return KArgMapInternal::to_string(m_value.timestamp.time_since_epoch());
    case KArgTypes::duration:
      return KArgMapInternal::to_string(m_value.duration);
    default:
      return "";
    }
  }

  operator std::string() const {
    switch (m_type) {
    case KArgTypes::string:
      return std::string(m_value.string);
    default:
      std::string s;
      s.reserve(256);
      KArgMapInternal::argVariantToString(s, *this);
      return s;
    }
  }

  /**
   * \brief cast the value held in KArgVariant to the specified type.  Use of
   * this method is discouraged due to lack of type safety and should only be
   * used by serializers.
   *
   * Usage: val.as<int32_t>()
   */
  template <typename T> T as() const {
    return *reinterpret_cast<const T *>(&m_value);
  }

private:
  // Conversions helpers
  template <typename SourceType, typename DestType,
            typename std::enable_if<(std::is_signed<SourceType>::value)>::type
                * = nullptr>
  DestType convert_number_with_default_mixed_sign(SourceType value,
                                                  DestType defaultValue)
      const { // std::cout << "signed source, unsiged dest\n";
    if (value < 0)
      return defaultValue;
    return (typename std::make_unsigned<DestType>::type(value) >
            std::numeric_limits<DestType>::max())
               ? defaultValue
               : DestType(value);
  }

  template <typename SourceType, typename DestType,
            typename std::enable_if<
                (std::is_unsigned<SourceType>::value &&
                 std::is_floating_point<DestType>::value)>::type * = nullptr>
  DestType convert_number_with_default_mixed_sign(SourceType value,
                                                  DestType defaultValue)
      const { // std::cout << "unsigned source, floating dest\n";
    return DestType(value);
  }

  template <typename SourceType, typename DestType,
            typename std::enable_if<
                (std::is_unsigned<SourceType>::value &&
                 !std::is_floating_point<DestType>::value)>::type * = nullptr>
  DestType convert_number_with_default_mixed_sign(SourceType value,
                                                  DestType defaultValue)
      const { // std::cout << "unsigned source, signed non float dest\n";
    return (value > typename std::make_unsigned<DestType>::type(
                        std::numeric_limits<DestType>::max()))
               ? defaultValue
               : DestType(value);
  }

  //=============================================
  template <typename SourceType, typename DestType,
            typename std::enable_if<(std::is_unsigned<SourceType>::value ==
                                     std::is_unsigned<DestType>::value)>::type
                * = nullptr>
  DestType convert_number_with_default_signed_check(
      SourceType value,
      DestType defaultValue) const { // std::cout << "same sign\n";
    return (value > std::numeric_limits<DestType>::max() ||
            value < std::numeric_limits<DestType>::lowest())
               ? defaultValue
               : DestType(value);
  }

  template <typename SourceType, typename DestType,
            typename std::enable_if<(std::is_unsigned<SourceType>::value !=
                                     std::is_unsigned<DestType>::value)>::type
                * = nullptr>
  DestType convert_number_with_default_signed_check(
      SourceType value,
      DestType defaultValue) const { // dispatch to mixed sign checks
    return convert_number_with_default_mixed_sign(value, defaultValue);
  }

  //=============================
  template <typename SourceType, typename DestType,
            typename std::enable_if<
                (std::is_same<SourceType, DestType>::value)>::type * = nullptr>
  DestType convert_number_with_default(SourceType value, DestType defaultValue)
      const { // std::cout << "same type\n";
    return DestType(value);
  }

  template <typename SourceType, typename DestType,
            typename std::enable_if<
                (!std::is_same<SourceType, DestType>::value &&
                 std::is_same<DestType, bool>::value)>::type * = nullptr>
  DestType convert_number_with_default(SourceType value, DestType defaultValue)
      const { // std::cout << "dest bool\n";
    return DestType(value != 0);
  }

  template <typename SourceType, typename DestType,
            typename std::enable_if<
                (!std::is_same<SourceType, DestType>::value &&
                 std::is_same<SourceType, bool>::value)>::type * = nullptr>
  DestType convert_number_with_default(SourceType value, DestType defaultValue)
      const { // std::cout << "src bool\n";
    return DestType(value ? 1 : 0);
  }

  template <typename SourceType, typename DestType,
            typename std::enable_if<
                (!std::is_same<SourceType, DestType>::value &&
                 !std::is_same<bool, DestType>::value &&
                 !std::is_same<SourceType, bool>::value)>::type * = nullptr>
  DestType convert_number_with_default(SourceType value, DestType defaultValue)
      const { // dispatch to signed checks
    return convert_number_with_default_signed_check(value, defaultValue);
  }

  //===========================================

  template <typename SourceType, typename DestType>
  DestType convert_number(DestType defaultValue) const {
    auto val = *reinterpret_cast<const SourceType *>(&m_value);
    return convert_number_with_default(val, defaultValue);
  }

  template <typename T> void vec_reset() {
    typedef std::shared_ptr<std::vector<T>> VecType;
    VecType *vec = reinterpret_cast<VecType *>(&m_value);
    vec->reset();
    vec->~VecType();
    // http://stackoverflow.com/questions/11988209/call-explicit-constructor-destructor-with-traits-in-templatized-function
  }

  void reset() {
    if (m_type == KArgTypes::null)
      return;
    if (m_vector) {
      switch (m_type) {
      case KArgTypes::int8:
        return vec_reset<int8_t>();
      case KArgTypes::int16:
        return vec_reset<int16_t>();
      case KArgTypes::int32:
        return vec_reset<int32_t>();
      case KArgTypes::int64:
        return vec_reset<int64_t>();
      case KArgTypes::uint8:
        return vec_reset<uint8_t>();
      case KArgTypes::uint16:
        return vec_reset<uint16_t>();
      case KArgTypes::uint32:
        return vec_reset<uint32_t>();
      case KArgTypes::uint64:
        return vec_reset<uint64_t>();
      case KArgTypes::float32:
        return vec_reset<float>();
      case KArgTypes::float64:
        return vec_reset<double>();
      case KArgTypes::cfloat32:
        return vec_reset<std::complex<float>>();
      case KArgTypes::cfloat64:
        return vec_reset<std::complex<double>>();
      case KArgTypes::string:
        return vec_reset<std::string>();
      case KArgTypes::map:
        return vec_reset<k_arg_map_type>();
      case KArgTypes::list:
        return vec_reset<k_arg_list_type>();
      default:
        break;
      }
    } else {
      switch (m_type) {
      case KArgTypes::map: {
        m_value.map.~k_arg_map_ptr();
        break;
      }
      case KArgTypes::list: {
        m_value.list.~k_arg_list_ptr();
        break;
      }
      case KArgTypes::custom: {
        m_value.custom.~k_arg_custom_ptr();
        break;
      }
      case KArgTypes::string: {
        m_value.string.~k_map_string_t();
        break;
      }
      default:
        break;
      }
    }
    m_type = KArgTypes::null;
  }
}; // KArgVariant

///< A null KArgVariant for internal use
typedef KArgMapInternal::Singleton<KArgVariant> NullKArgVariant;

template <typename T>
void vec_string(std::string &s, const KArgVariant &val,
                bool addQuotes = false) {
  const auto &vec =
      *reinterpret_cast<const std::shared_ptr<std::vector<T>> *>(&val.m_value);
  s.append("[");
  for (size_t i = 0; i < vec->size(); ++i) {
    if (i != 0)
      s.append(",");
    if (addQuotes)
      s.append("\"");
    s.append(KArgMapInternal::to_string((*vec)[i]));
    if (addQuotes)
      s.append("\"");
  }
  s.append("]");
}

template <typename T>
void vec_string_iter(std::string &s, const KArgVariant &val,
                     std::function<void(T &)> func) {
  const auto &vec =
      *reinterpret_cast<const std::shared_ptr<std::vector<T>> *>(&val.m_value);
  s.append("[");
  for (size_t i = 0; i < vec->size(); ++i) {
    if (i != 0)
      s.append(",");
    auto element = (*vec)[i];
    func(element);
  }
  s.append("]");
}

namespace KArgMapInternal {
inline void argListToString(std::string &s, const k_arg_list_type &list) {
  s.append("[");
  bool first = true;
  for (auto const &item : list) {
    if (!first) {
      s.append(",");
    }
    first = false;
    KArgMapInternal::argVariantToString(s, item);
  }
  s.append("]");
}

inline void argMapToString(std::string &s, const k_arg_map_type &map) {
  s.append("{");
  bool first = true;
  for (auto const &item : map) {
    if (item.second.m_type == KArgTypes::null) {
      continue;
    }
    s.append(first ? "\"" : ", \"");
    first = false;
    s.append(item.first);
    s.append("\":");
    KArgMapInternal::argVariantToString(s, item.second);
  }
  s.append("}");
}

inline void argVariantToString(std::string &s, const KArgVariant &val) {
  if (val.m_vector) {
    switch (val.m_type) {
    case KArgTypes::boolean: {
      const auto &vec =
          *reinterpret_cast<const std::shared_ptr<std::vector<bool>> *>(
              &val.m_value);
      s.append("[");
      for (size_t i = 0; i < vec->size(); ++i) {
        if (i != 0)
          s.append(",");
        s.append((*vec)[i] ? "true" : "false");
      }

      s.append("]");
      return;
    }
    case KArgTypes::int8:
      return vec_string<int8_t>(s, val);
    case KArgTypes::int16:
      return vec_string<int16_t>(s, val);
    case KArgTypes::int32:
      return vec_string<int32_t>(s, val);
    case KArgTypes::int64:
      return vec_string<int64_t>(s, val);
    case KArgTypes::uint8:
      return vec_string<uint8_t>(s, val);
    case KArgTypes::uint16:
      return vec_string<uint16_t>(s, val);
    case KArgTypes::uint32:
      return vec_string<uint32_t>(s, val);
    case KArgTypes::uint64:
      return vec_string<uint64_t>(s, val);
    case KArgTypes::float32:
      return vec_string<float>(s, val);
    case KArgTypes::float64:
      return vec_string<double>(s, val);
    case KArgTypes::cfloat32:
      return vec_string<std::complex<float>>(s, val, true);
    case KArgTypes::cfloat64:
      return vec_string<std::complex<double>>(s, val, true);
    case KArgTypes::timestamp:
      return vec_string<KTimestamp>(s, val);
    case KArgTypes::duration:
      return vec_string<KDuration>(s, val);
    case KArgTypes::string:
      return vec_string<std::string>(s, val, true);
    case KArgTypes::map:
      return vec_string_iter<KMapBase>(
          s, val, [&s](KMapBase &v) { argMapToString(s, *v.m_map); });
    case KArgTypes::list:
      return vec_string_iter<KListBase>(
          s, val, [&s](KListBase &v) { argListToString(s, *v.m_list); });
    default:
      break;
    }
    return;
  }
  switch (val.m_type) {
  case KArgTypes::null:
    s.append("null");
    break;
  case KArgTypes::map: {
    auto map = val.m_value.map;
    argMapToString(s, *map);
    break;
  }

  case KArgTypes::list: {
    argListToString(s, *val.m_value.list);
    break;
  }

  case KArgTypes::string: {
    s.append("\"");
    s.append(val.m_value.string);
    s.append("\"");
    break;
  }

  case KArgTypes::boolean: {
    s.append(val.m_value.boolean ? "true" : "false");
    break;
  }
  case KArgTypes::custom: {
    argMapToString(s, *customArgToString(val));
    break;
  }
  default:
    s.append(val.scalar_to_string());
    break;
  }
}
} // namespace KArgMapInternal

inline std::ostream &operator<<(std::ostream &o, const k_arg_list_ptr val) {
  if (val) {
    std::string s;
    s.reserve(256);
    KArgMapInternal::argListToString(s, *val);
    o << s;
  } else {
    o << std::string("[]");
  }
  return o;
}

inline std::ostream &operator<<(std::ostream &o, const k_arg_map_ptr val) {
  if (val) {
    std::string s;
    s.reserve(256);
    KArgMapInternal::argMapToString(s, *val);
    o << s;
  } else {
    o << std::string("{}");
  }
  return o;
}

class KArgList : public KListBase {
  friend class KArgMap;
  friend class KArgMapTest;
  friend class KArgMapSerializer;

  KArgList(std::shared_ptr<k_arg_list_type> list) { m_list = list; }

public:
  KArgList() { m_list = std::make_shared<k_arg_list_type>(); }

  KArgList(std::initializer_list<KArgVariant> l) {
    m_list = std::make_shared<k_arg_list_type>(l);
  }

  KArgList(KArgVariant &v) {
    if (v.m_type == KArgTypes::list) {
      m_list = v.m_value.list;
    } else {
      m_list = std::make_shared<k_arg_list_type>();
    }
  }

  void add(std::initializer_list<KArgVariant> l) {
    m_list->insert(m_list->end(), l);
  }

  void add(KArgList list) {
    m_list->insert(m_list->end(), list.m_list->begin(), list.m_list->end());
  }

  KArgVariant &operator[](const size_t index) {
    // Beware, exception if element does not exist
    return m_list->operator[](index);
  }

  /**
   * \brief Get a typed value from the KArgMap.
   * \tparam T The type to return.
   * \param index The index to look up.
   * \param defaultValue The value to return if the key is not present.
   * \return The value associated with key or the default value parameter.
   */
  template <typename T> T get(const size_t index, T defaultValue) {
    if (index > m_list->size())
      return defaultValue;
    auto val = m_list->operator[](index);
    return val.get(defaultValue);
  }

  // get access
  std::string get(const size_t index, const char *defaultValue) {
    if (index > m_list->size())
      return defaultValue;
    auto val = m_list->operator[](index);
    return val.get(defaultValue);
  }

  // TODO: support std::shared_ptr<const T> getCustomType
  template <typename T> T get(const size_t index, T defaultValue) const {
    if (index > m_list->size())
      return defaultValue;
    auto val = m_list->operator[](index);
    return val.get(defaultValue);
  }

  // get access
  const std::string get(const size_t index, const char *defaultValue) const {
    if (index > m_list->size())
      return defaultValue;
    auto val = m_list->operator[](index);
    return val.get(defaultValue);
  }

#ifndef K_CUSTOM_TYPES_UNSUPPORTED
  template <typename T>
  T getCustomType(const size_t index, T defaultValue = T()) {
    if (index > m_list->size())
      return defaultValue;
    auto val = m_list->operator[](index);
    return val.getCustomType(defaultValue);
  }

  // TODO: support std::shared_ptr<const T> getCustomType
  template <typename T>
  const T getCustomType(const size_t index, T defaultValue = T()) const {
    if (index > m_list->size())
      return defaultValue;
    auto val = m_list->operator[](index);
    return val.getCustomType(defaultValue);
  }
#endif

  template <typename T>
  typename std::enable_if<KArgMapInternal::is_k_type<T>::value, void>::type
  set(const size_t index, T value) {
    m_list->operator[](index) = value;
  }

  template <typename T>
  void set(const size_t index, const std::shared_ptr<const T> value) {
    static_assert(sizeof(T) != sizeof(T), "Cannot store const element.");
  }

  template <typename T> void set(const size_t index, std::vector<T> &&value) {
    m_list->operator[](index) = std::move(value);
  }

  template <typename T> void set(const size_t index, std::vector<T> &value) {
    static_assert(
        sizeof(T) != sizeof(T),
        "to set a std::vector, wrap with std::move or std::shared_ptr.");
  }

  operator std::string() const {
    std::string s;
    s.reserve(256);
    KArgMapInternal::argListToString(s, *m_list);
    return s;
  }

  size_t use_count() const noexcept { return m_list.use_count(); }

  // delegated methods
  size_t size() const { return m_list->size(); }

  bool empty() const { return m_list->empty(); }

  void clear() { return m_list->clear(); }

  decltype(m_list->begin())
  begin() K_NOEXCEPT { // return iterator for beginning of mutable sequence
    return (m_list->begin());
  }

  decltype(m_list->begin()) begin() const
      K_NOEXCEPT { // return iterator for beginning of nonmutable sequence
    return (m_list->begin());
  }

  decltype(m_list->end())
  end() K_NOEXCEPT { // return iterator for end of mutable sequence
    return (m_list->end());
  }

  decltype(m_list->end())
  end() const K_NOEXCEPT { // return iterator for end of nonmutable sequence
    return (m_list->end());
  }

  void push_back(KArgVariant &&_Val) { m_list->push_back(_Val); }

  void add(KArgVariant &&_Val) { m_list->push_back(_Val); }

  template <typename T>
  typename std::enable_if<KArgMapInternal::is_k_type<T>::value, void>::type
  add(T value) {
    m_list->emplace_back(value);
  }

  template <typename T> void addCustomType(T value) {
    auto ptr = std::make_shared<KArgCustomType<T>>(value);
    m_list->push_back(std::move(ptr));
  }

  void removeAt(size_t index) {
    if (index < m_list->size()) {
      m_list->erase(m_list->begin() + index);
    }
  }

  KArgList deepClone() const {
    KArgList result = KArgMapInternal::k_arg_list_clone(m_list);
    return result;
  }

  friend inline std::ostream &operator<<(std::ostream &o, const KArgList &arg);
}; // KArgList

inline std::ostream &operator<<(std::ostream &o, const KArgList &arg) {
  o << arg.m_list;
  return o;
}

class KArgMap : public KMapBase {
  friend class KArgList;
  friend class KArgMapTest;
  friend class KArgMapSerializer;

public:
  KArgMap() { m_map = std::make_shared<k_arg_map_type>(); }

  KArgMap(std::initializer_list<std::pair<const std::string, KArgVariant>> l) {
    m_map = std::make_shared<k_arg_map_type>(l);
  }

  KArgMap(KArgVariant &v) {
    if (v.m_type == KArgTypes::map) {
      m_map = v.m_value.map;
    } else {
      m_map = std::make_shared<k_arg_map_type>();
    }
  }

  void add(std::initializer_list<std::pair<const std::string, KArgVariant>> l) {
    m_map->insert(l.begin(), l.end());
  }

  bool containsKey(const std::string key) const {
    return !(m_map->find(key) == m_map->end());
  }

  // String get methods with both const char * and std::string
  template <typename T>
  typename std::enable_if<(std::is_same<T, const char *>::value ||
                           std::is_same<T, std::string>::value),
                          std::string>::type
  get(const std::string &key, T defaultValue) const {
    return const_cast<KArgMap *>(this)->get_impl<std::string>(key,
                                                              defaultValue);
  }

  template <typename T>
  typename std::enable_if<(std::is_same<T, const char *>::value ||
                           std::is_same<T, std::string>::value),
                          std::string>::type
  get(const std::string &key, T defaultValue) {
    return get_impl<std::string>(key, defaultValue);
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, KArgMap>::value, KArgMap>::type
  get(const std::string &key, T defaultValue) {
    return get_impl(key, defaultValue.m_map);
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, KArgMap>::value, const KArgMap>::type
  get(const std::string &key, T defaultValue) const {
    return const_cast<KArgMap *>(this)->get_impl<T>(key, defaultValue.m_map);
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, KArgList>::value, KArgList>::type
  get(const std::string &key, T defaultValue) {
    return get_impl<>(key, defaultValue.m_list);
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, KArgList>::value,
                          const KArgList>::type
  get(const std::string &key, T defaultValue) const {
    return const_cast<KArgMap *>(this)->get_impl<T>(key, defaultValue.m_list);
  }

  /**
   * \brief Get a typed value from the KArgMap.
   * \tparam T The type to return.
   * \param key The key name to look up.
   * \param defaultValue The value to return if the key is not present.
   * \return The value associated with key or the default value parameter.
   */
  template <typename T>
  typename std::enable_if<(!std::is_class<T>::value &&
                           !std::is_pointer<T>::value) ||
                              std::is_same<T, std::complex<float>>::value ||
                              std::is_same<T, std::complex<double>>::value ||
                              std::is_same<T, KTimestamp>::value ||
                              std::is_same<T, KDuration>::value,
                          T>::type
  get(const std::string &key, T defaultValue) {
    return get_impl(key, defaultValue);
  }

  template <typename T>
  typename std::enable_if<(!std::is_class<T>::value), T>::type
  get(const std::string &key) {
    static_assert(sizeof(T) != sizeof(T),
                  "Get operations require a default value to be provided.");
    return const_cast<KArgMap *>(this)->get_impl(key, T(0));
  }

  /**
   * \brief Get a typed value from the KArgMap.
   * \tparam T The type to return.
   * \param key The key name to look up.
   * \param defaultValue The value to return if the key is not present.
   * \return The value associated with key or the default value parameter.
   */
  template <typename T>
  typename std::enable_if<(!std::is_class<T>::value &&
                           !std::is_pointer<T>::value) ||
                              std::is_same<T, std::complex<float>>::value ||
                              std::is_same<T, std::complex<double>>::value ||
                              std::is_same<T, KTimestamp>::value ||
                              std::is_same<T, KDuration>::value,
                          T>::type
  get(const std::string &key, T defaultValue) const {
    return const_cast<KArgMap *>(this)->get_impl(key, defaultValue);
  }

  template <typename T>
  typename std::enable_if<(!std::is_class<T>::value ||
                           std::is_same<std::string, T>::value),
                          T>::type
  get(const std::string &key) const {
    static_assert(sizeof(T) != sizeof(T),
                  "Get operations require a default value to be provided.");
    return const_cast<KArgMap *>(this)->get_impl(key, T(0));
  }

  /**
   * \brief Get a typed value from the KArgMap for a type that returns a smart
   * pointer. \tparam T The type to return. \param key The key name to look up.
   * \param defaultValue The value to return if the key is not present.  Default
   * is an empty smart pointer. \return The value associated with key or the
   * default value parameter.
   */
  template <typename T>
  typename std::enable_if<!std::is_same<std::string, T>::value &&
                              std::is_class<T>::value,
                          std::shared_ptr<T>>::type
  get(const std::string &key,
      std::shared_ptr<T> defaultValue = std::make_shared<T>()) {
    return get_impl(key, defaultValue);
  }

  template <typename T>
  typename std::enable_if<!std::is_same<std::string, T>::value &&
                              std::is_class<T>::value,
                          std::shared_ptr<const T>>::type
  get(const std::string &key, std::shared_ptr<const T> defaultValue =
                                  std::make_shared<const T>()) const {
    // hokey workaround as I couldn't figure hot to cast to std::shared_ptr<T>
    // for get_impl call
    auto x = std::make_shared<T>();
    auto value =
        const_cast<KArgMap *>(this)->get_impl<std::shared_ptr<T>>(key, x);
    if (x == value)
      return defaultValue;
    return value;
  }

  KArgVariant &operator[](const std::string &key) {
    // Beware, the array operator will create a key entry if it does not exist.
    return m_map->operator[](key);
  }

  const KArgVariant &operator[](const std::string &key) const {
    auto val = m_map->find(key);
    if (val == m_map->end()) {
#ifdef K_NOEXECEPT
      throw std::exception("Key not found in const KArgMap");
#else
      // Not in list, useless to caller, no exceptions so fall thru and see what
      // happens
#endif
    }
    return m_map->operator[](key);
  }

  // Set operations

#if 0
            template <typename T>
            void set(const std::string& key, T value) const
            {
                static_assert(always_false<T>::value, "Cannot set values in a const container");
            }
#endif

  template <typename T>
  typename std::enable_if<KArgMapInternal::is_k_type<T>::value, void>::type
  set(const std::string &key, T value) {
    auto pair = m_map->find(key);
    if (pair != m_map->end()) {
      // key already exists, replace value
      pair->second = value;
    } else {
      // the key does not exist in the map.
      // check to see if it contains a path and utilize the path if so.
      // If the key is not a path, insert immediately.
      auto ckey = key.c_str();
      while (*ckey && *ckey != '|') {
        ckey++;
      }
      if (*ckey) {
        KArgVariant &item = getByPath(key, *this, true);
        item = value;
      } else {
        // Use lb as a hint to insert, so it can avoid another lookup
        m_map->emplace(key, value);
      }
    }
  }

  template <typename T>
  void set(const std::string &key, const std::shared_ptr<const T> value) {
    static_assert(sizeof(T) != sizeof(T), "Cannot store const element.");
  }

  template <typename T>
  void set(const std::string &key, std::vector<T> &&value) {
    (*m_map)[key] = std::move(value);
  }

  template <typename T>
  void set(const std::string &key, std::vector<T> &value) {
    static_assert(
        sizeof(T) != sizeof(T),
        "to set a std::vector, wrap with std::move or std::shared_ptr.");
  }

#ifndef K_CUSTOM_TYPES_UNSUPPORTED
  template <typename T> void setCustomType(const std::string &key, T value) {
    auto ptr = std::make_shared<KArgCustomType<T>>(value);
    (*m_map)[key] = std::move(ptr);
  }

  template <typename T>
  T getCustomType(const std::string &key, T defaultValue = T()) {
    auto val = m_map->find(key);
    if (val == m_map->end() || (val->second.m_type != KArgTypes::custom &&
                                val->second.m_type != KArgTypes::map))
      return defaultValue;
    return val->second.getCustomType(defaultValue);
  }
#endif

  // to string support
  operator std::string() const { return to_string(); }

  std::string to_string() const {
    std::string s;
    s.reserve(256);
    KArgMapInternal::argMapToString(s, *m_map);
    return s;
  }

  // delegated methods
  size_t erase(const std::string key) const { return m_map->erase(key); }

  size_t size() const { return m_map->size(); }

  bool empty() const { return m_map->empty(); }

  void clear() { return m_map->clear(); }

  decltype(m_map->begin())
  begin() K_NOEXCEPT { // return iterator for beginning of mutable sequence
    return (m_map->begin());
  }

  decltype(m_map->begin()) begin() const
      K_NOEXCEPT { // return iterator for beginning of nonmutable sequence
    return (m_map->begin());
  }

  decltype(m_map->end())
  end() K_NOEXCEPT { // return iterator for end of mutable sequence
    return (m_map->end());
  }

  decltype(m_map->end())
  end() const K_NOEXCEPT { // return iterator for end of nonmutable sequence
    return (m_map->end());
  }

  size_t use_count() const K_NOEXCEPT { return m_map.use_count(); }

  KArgMap deepClone() const {
    KArgMap map = KArgMapInternal::k_arg_map_clone(m_map);
    return map;
  }

protected:
  KArgMap(std::shared_ptr<k_arg_map_type> map) { m_map = map; }

private:
  /**
   * \brief Given a path and a KArgVariant node, examine the key to determine if
   * the node is required to be a list or a map.   If createPath==true nodes
   * will be adjusted to satisfy the key hint.  If createPath==false then a null
   * element is returned if a node is discovered of the incorrerct type. \param
   * path The container path to traverse. \param item The candidate node. \param
   * createPath True to allow the elements to be created or adjusted. \return
   * The KArgVariant node associated with the path.  A null KArgVariant is
   * returned if the path does not resolve to a valid node.
   */
  KArgVariant &getByPath(const std::string &path, KArgVariant &item,
                         bool createPath) {
    auto isNumeric = path[0] >= '0' && path[0] <= '9';
    if (isNumeric) {
      // need a list
      if (createPath) {
        if (item.m_type == KArgTypes::null) {
          item = KArgList();
        }
      }
      if (item.m_type == KArgTypes::list && !item.m_vector) {
        KArgList temp = item.m_value.list;
        return getByPath(path, temp, createPath);
      }
      return NullKArgVariant::Instance();
    } else {
      // need a map
      if (createPath) {
        if (item.m_type == KArgTypes::null) {
          item = KArgMap();
        } else if (item.m_type != KArgTypes::map) {
          KArgMap m;
          m["value"] = std::move(item);
          item = m;
        }
      }

      if (item.m_type == KArgTypes::map && !item.m_vector) {
        KArgMap m = item.m_value.map;
        return getByPath(path, m, createPath);
      }
      return NullKArgVariant::Instance();
    }
  }

  /**
   * \brief Find the node associated with the path string.
   * \param path The path to traverse to find the specified node.
   * \param list The initial container to being the search from.
   * \param createPath If true, create containers as needed while traversing
   * path. \return The KArgVariant node associated with the path.  A null
   * KArgVariant is returned if the path does not resolve to a valid node.
   */
  KArgVariant &getByPath(const std::string path, KArgList &list,
                         bool createPath) {
    const char *ckey = path.c_str();
    const char *key1 = ckey;
    while (*ckey && *ckey != '|') {
      ckey++;
    }
    if (*ckey != '|') {
      auto index =
          key1[0] >= '0' && key1[0] <= '9' ? ::strtoull(key1, nullptr, 10) : 0;
      if (createPath) {
        for (size_t i = list.size(); i <= index; i++)
          list.m_list->insert(list.m_list->end(), KArgVariant());
      }
      if (index >= list.size()) {
        return NullKArgVariant::Instance();
      }
      return list.m_list->operator[](uint32_t(index));
    }

    auto pos = ckey - path.c_str();
    auto keyfirst = std::string(path, 0, pos);
    auto keylast = path.substr(pos + 1);
    auto index =
        key1[0] >= '0' && key1[0] <= '9' ? ::strtoull(key1, nullptr, 10) : 0;

    if (createPath) {
      for (size_t i = list.size(); i <= index; i++)
        list.m_list->insert(list.m_list->end(), KArgVariant());
    }
    if (index >= list.size()) {
      return NullKArgVariant::Instance();
    }
    KArgVariant &item = list.m_list->operator[](uint32_t(index));
    return getByPath(keylast, item, createPath);
  }

  /**
   * \brief Find the node associated with the path string.
   * \param path The path to traverse to find the specified node.
   * \param map The initial container to being the search from.
   * \param createPath If true, create containers as needed while traversing
   * path. \return The KArgVariant node associated with the path.  A null
   * KArgVariant is returned if the path does not resolve to a valid node.
   */
  KArgVariant &getByPath(const std::string path, KArgMap &map,
                         bool createPath) {
    const char *ckey = path.c_str();
    while (*ckey && *ckey != '|') {
      ckey++;
    }
    if (*ckey != '|') {
      return map.m_map->operator[](path);
    }

    auto pos = ckey - path.c_str();
    auto keyfirst = std::string(path, 0, pos);
    auto keylast = path.substr(pos + 1);
    KArgVariant &item = map.m_map->operator[](keyfirst);

    return getByPath(keylast, item, createPath);
  }

  template <typename T> T get_impl(const std::string &key, T defaultValue) {
    auto val = m_map->find(key);
    bool usePath = false;
    if (val == m_map->end()) {
      // Look for path character
      auto pos = key.find('|');
      if (pos == std::string::npos)
        return defaultValue;
      usePath = true;
    }

    KArgVariant &item = usePath ? getByPath(key, *this, false) : val->second;
    if (item.m_type == KArgTypes::null)
      return defaultValue;
    if (item.m_type == KArgTypes::map &&
        KArgMapInternal::k_type_info<T>::type_code != KArgTypes::map) {
      // The get request is for something other than a
      // map. Descend into the map looking for 'value'.
      KArgMap temp(item.m_value.map);
      return temp.get_impl("value", defaultValue);
    }
    return item.get(defaultValue);
  }

  friend std::ostream &operator<<(std::ostream &o, const KArgMap &arg);
  friend KArgVariant
  KArgMapInternal::argMapToCustomType(const std::string typeName,
                                      k_arg_map_ptr map);
};

namespace KArgMapInternal {
inline k_arg_list_ptr k_arg_list_clone(const k_arg_list_ptr from) {
  k_arg_list_ptr result = std::make_shared<k_arg_list_type>();
  k_arg_list_type &list = *result;
  for (auto &item : *from) {
    switch (item.m_type) {
    case KArgTypes::map: {
      list.push_back(k_arg_map_clone(item.m_value.map));
      break;
    }
    case KArgTypes::list:
      list.push_back(k_arg_list_clone(item.m_value.list));
      break;
    default:
      list.push_back(item);
    }
  }
  return result;
}

inline k_arg_map_ptr k_arg_map_clone(const k_arg_map_ptr from) {
  k_arg_map_ptr result = std::make_shared<k_arg_map_type>();
  k_arg_map_type &map = *result;
  for (auto &item : *from) {
    auto key = item.first;
    switch (item.second.m_type) {
    case KArgTypes::map: {
      map[key] = k_arg_map_clone(item.second.m_value.map);
      break;
    }
    case KArgTypes::list:
      map[key] = k_arg_list_clone(item.second.m_value.list);
      break;
    default:
      map[key] = item.second;
    }
  }
  return result;
}
} // namespace KArgMapInternal

inline std::ostream &operator<<(std::ostream &o, const KArgMap &arg) {
  o << arg.m_map;
  return o;
}

namespace KArgMapInternal {

class KArgConverterBase;

///< A list mapping type names to serialization helpers. Used when decoding.
typedef Singleton<std::map<std::string, KArgConverterBase *>>
    KArgConverterFromTypeName;
///< A list mapping c++ typeid to serialization helpers. Used when encoding.
typedef Singleton<std::map<std::string, KArgConverterBase *>>
    KArgConverterFromTypeCode;

class KArgConverterBase {
  friend class KArgUtility;

protected:
  virtual ~KArgConverterBase() = default;

public:
  virtual KArgMap toArgMap(const void *value) = 0;
  virtual KArgVariant fromArgMap(const KArgMap &map) = 0;

  static const std::string &customKeyName() {
    // Initialize the static variable
    static std::string codable("{{type}}");
    return codable;
  }

  std::string name;
  std::string typeCode;
  const std::string getName() { return name; }

  KArgConverterBase(const std::string name, const std::string typeCode)
      : name(name), typeCode(typeCode) {
    KArgConverterFromTypeName::Instance()[name] = this;
    KArgConverterFromTypeCode::Instance()[typeCode] = this;
  }
};

template <typename CustomType, typename Converter>
class KArgConverter : public KArgConverterBase {
  KArgMap toArgMap(const void *value) override {
    return CustomType::toArgMap(static_cast<const CustomType *>(value));
  }

  KArgVariant fromArgMap(const KArgMap &map) override {
    auto ptr = std::make_shared<KArgCustomType<CustomType>>(
        CustomType::fromArgMap(map));
    return ptr;
  }

  KArgConverter()
      : KArgConverterBase(Converter::argMapTypeName(),
                          KARG_TYPE_ID_NAME(CustomType)) {}

public:
  static void registerType() {
    static KArgConverter instance; // side affect: add to lookup lists
  }
};
} // namespace KArgMapInternal

/**
 * \brief A Utility class to assist with serializing custom data types.
 */
class KArgUtility {
public:
  /**
   * \brief Register a custom type with the KArgMap serializer helpers.  Custom
   * types are serialized by converting them to or from a KArgMap.  In order to
   * support serialization the custom type must provide a unique class name such
   * as "Company::Module::Class", the custom class type, and a class that
   * implements the two methods required for serialization.  The toArgMap() and
   * fromArgMap() methods must be static methods can can be either in a helper
   * class or in the custom type itself.
   *
   * The two static methods required by the Serializer type are:
   *
   * static KArgMap toArgMap(const MyCustomType *value);
   *
   * static MyCustomType fromArgMap(const KArgMap& map);
   *
   * \tparam CustomType The custom type to be registered with KArgMap.
   * \tparam Serializer The serializer type that contains the toArgMap(),
   * fromArgMap(), and argMapTypeName() methods. This can be the same type as
   * CustomType. \return An integer suitable for assigment in a static global
   * variable.
   */
  template <typename CustomType, typename Serializer = CustomType>
  static int registerCustomType() {
    KArgMapInternal::KArgConverter<CustomType, Serializer>::registerType();
    return 1;
  }

  /**
   * \brief A static helper method used by serializers to encode custom types.
   * \param arg The KArgVariant value contained within a KArgMap container.
   * \return A KArgMap with key/value pairs representing the custom type.
   */
  static KArgMap toArgMap(const KArgVariant &arg) {
    if (arg.m_type != KArgTypes::custom)
      return KArgMap();
    auto converter = KArgMapInternal::KArgConverterFromTypeCode::Instance()
        [arg.m_value.custom->m_name];
    if (!converter)
      return KArgMap(); // TODO: Possibly fill with meta data about the custom
                        // type
    auto value = converter->toArgMap(arg.m_value.custom->m_value);
    value.set(customKeyName(), converter->getName());
    return value;
  }

  /**
   * \brief  A static helper method used by serializers to decode custom types.
   * \param map A may representing the custom type.
   * \return A KArgVariant instance suitable for placement into a KArgMap
   * container.
   */
  static KArgVariant fromArgMap(const KArgMap &map) {
    std::string typeName = map.get(customKeyName(), "");
    if (typeName.length() == 0)
      return KArgVariant(map.m_map);
    auto converter =
        KArgMapInternal::KArgConverterFromTypeName::Instance()[typeName];
    if (!converter)
      return KArgVariant(map.m_map);
    return converter->fromArgMap(map);
  }

  /**
   * \brief Return a string representing the custom key name used by serializers
   * to indicate a serialized KArgMap is a custom type.  The value for this key
   * will be the string registered with the registerCustomType() method.
   * \return
   */
  static const std::string &customKeyName() {
    return KArgMapInternal::KArgConverterBase::customKeyName();
  }
};

namespace KArgMapInternal {
inline k_arg_map_ptr customArgToString(const KArgVariant &val) {
  auto map = KArgUtility::toArgMap(val);
  return map.m_map;
}

inline KArgVariant argMapToCustomType(const std::string typeName,
                                      k_arg_map_ptr map) {
  KArgMap m = map;
  if (m.containsKey(KArgUtility::customKeyName())) {
    // If a type name is present, use it instead of argument
    return KArgUtility::fromArgMap(m);
  }
  auto converter =
      KArgMapInternal::KArgConverterFromTypeName::Instance()[typeName];
  if (!converter)
    return KArgVariant();
  return converter->fromArgMap(m);
}
} // namespace KArgMapInternal
} // namespace entazza
