// SPDX-License-Identifier: BSD-3-Clause
#pragma once

namespace entazza {
// Forwared decl for serializer declarations
class CborSerializer;
} // namespace entazza

#if defined(KArgMapSerializer) || defined(MicroCborSerializer)
#error CborSerializer.h must be included before KArgMap.hpp and MicroCbor.hpp to define serializer friend classes
#endif

// Specify our custom serializer for internal access
#define KArgMapSerializer ::entazza::CborSerializer
#define MicroCborSerializer ::entazza::CborSerializer

#include "kargmap/KArgMap.hpp"
#include "microcbor/MicroCbor.hpp"
#include <memory>

namespace entazza {
class CborSerializer {

  using CborError_t = MicroCbor::Error;

private:
  MicroCbor cbor;

public:
  /**
   * @brief Construct a new CborSerializer object suitable for encoding a
   * KArgMap into CBOR.
   *
   * @param buf A pointer to memory to place a cbor encoded representation.
   * @param maxBufLen The size in bytes of the buf memory.
   */
  CborSerializer(void *buf, const uint32_t maxBufLen,
                 const bool nullTerminate = false)
      : cbor(buf, maxBufLen, nullTerminate) {}

  /**
   * @brief Encode a KArgMap instance into a CBOR binary array of bytes.
   *
   * @param argMap The KArgMap instance to serialize.
   * @return CborError_t 0 If non-zero the output buffer was not large enough.
   * In this case use the bytesNeeded() method to query how big the buffer needs
   * to be.
   */
  inline CborError_t encode(const KArgMap &argMap) {
    return encodeKArgMapImpl(*(argMap.m_map));
  }

  inline KArgMap decode() {
    auto info = cbor.getNextField();
    // We must be in a map to find anything
    if (info.majorval != entazza::kCborMap) {
      return KArgMap();
    }
    auto value = readItem("root");
    return value;
  };

  /**
   * @brief Get the result of encoding.
   * If non-zero the output buffer was not large enough.  In
   * this case use the bytesNeeded() method to query how big
   * the buffer needs to be.
   *
   * @return Error
   */
  inline CborError_t getResult() const noexcept { return cbor.getResult(); }

  /**
   * @brief Get the total number of bytes needed to encode the supplied fields.
   *
   * This number can be larger than bytesSerialized() if the buffer was not
   * large enough to hold the complete serialization.
   *
   * @return uint32_t
   */
  inline uint32_t bytesNeeded() const noexcept { return cbor.bytesNeeded(); }

  /**
   * @brief Get the total number of bytes serialized.
   *
   * @return uint32_t
   */
  inline uint32_t bytesSerialized() const noexcept {
    return cbor.bytesSerialized();
  }

  /**
   * @brief Reinitialize the working buffer.
   *
   * @param buf A pointer to a buffer
   * @param maxBufLen The length in bytes of the buffer
   */
  inline void initBuffer(void *buf, const uint32_t maxBufLen) noexcept {
    cbor.initBuffer(buf, maxBufLen);
  }

  /**
   * @brief Reinitialize the working buffer with a read-only buffer.
   *
   * This buffer can only be used for decoding cbor streams.
   *
   * @param buf A pointer to a buffer
   * @param maxBufLen The length in bytes of the buffer
   */
  inline void initBuffer(const void *buf, const uint32_t maxBufLen) noexcept {
    cbor.initBuffer(buf, maxBufLen);
  }

  /**
   * @brief Reset the encoder/decoder state to allow using again
   *
   */
  inline void restart() noexcept { cbor.restart(); }

private:
  inline void encodeArgList(const k_arg_list_type &argList) {
    cbor.encodeHeader(entazza::kCborArray, argList.size());
    for (auto const &item : argList) {
      encodeArgItem(nullptr, item);
    }
  }

  template <typename T>
  void vec_encode_iter(const KArgVariant &val, std::function<void(T &)> func) {
    const auto &vec =
        *reinterpret_cast<const std::shared_ptr<std::vector<T>> *>(
            &val.m_value.ptr);
    cbor.encodeTag(entazza::kCborTagHomogeneousArray);
    cbor.encodeHeader(entazza::kCborArray, vec->size());
    for (size_t i = 0; i < vec->size(); ++i) {
      auto element = (*vec)[i];
      func(element);
    }
  }

  /**
   * \brief Serialize a KArgVariant that contains a numeric array.
   * \tparam T The base numeric type in the array.
   * \param val The KArgVariant instance to serialize.
   */
  template <class T> void encodeArray(const KArgVariant &val) {
    const auto &vec =
        *reinterpret_cast<const std::shared_ptr<std::vector<T>> *>(
            &val.m_value.ptr);
    cbor.add(nullptr, vec->data(), vec->size(), false);
  }

  void encodeRawDuration(const KDuration &time) {
    int64_t t = time.count();
    int64_t nano = t % 1000000000;
    int64_t secs = t / 1000000000;
    cbor.encodeHeader(entazza::kCborMap, 2);
    cbor.encodeUInt8((entazza::kCborPosInt << 5) | 24, 1);
    cbor.encodeUInt32((entazza::kCborPosInt << 5) | 26, secs);
    cbor.encodeUInt8((entazza::kCborNegInt << 5) | 24, -1 - (-9));
    cbor.encodeUInt32((entazza::kCborPosInt << 5) | 26, nano);
  }

  inline void encodeDuration(const KDuration &time) {
    // tag 1002, map of len 1, key -9 (nanosec), uint ticks
    cbor.encodeTag(entazza::kCborTagDurationExt);
    encodeRawDuration(time);
  }

  inline void encodeTimestamp(const KTimestamp &timestamp) {
    // tag 1001, map of len 1, key -9 (nanosec), uint ticks
    cbor.encodeTag(entazza::kCborTagTimeExt);
    encodeRawDuration(timestamp.time_since_epoch());
  }

  inline void encodeString(const std::string &s) {
    cbor.add(nullptr, s.c_str());
  }

  /**
   * @brief Encode a basic KArgMap value.  If name is not null a map key is also
   * generated.
   *
   * @param name The optional name of a map element
   * @param val The value to encode
   */
  void encodeArgItem(const char *name, const KArgVariant &val) {
    cbor.encodeMapKey(name);
    if (val.m_vector) {
      switch (val.m_type) {
      case KArgTypes::boolean: {
        const auto &vec =
            *reinterpret_cast<const std::shared_ptr<std::vector<bool>> *>(
                &val.m_value.ptr);
        size_t length = vec->size();

        // TODO add this array type to MicroCbor perhaps
        cbor.encodeTag(entazza::kCborTagHomogeneousArray);
        cbor.encodeHeader(entazza::kCborArray, length);
        cbor.reserveBytes(length);
        for (size_t i = 0; i < length; ++i) {
          auto element = (*vec)[i];
          cbor.storeByte(element ? entazza::kCborTrue : entazza::kCborFalse);
        }
        return;
      }

      case KArgTypes::int8:
        return encodeArray<int8_t>(val);
      case KArgTypes::int16:
        return encodeArray<int16_t>(val);
      case KArgTypes::int32:
        return encodeArray<int32_t>(val);
      case KArgTypes::int64:
        return encodeArray<int64_t>(val);
      case KArgTypes::uint8:
        return encodeArray<uint8_t>(val);
      case KArgTypes::uint16:
        return encodeArray<uint16_t>(val);
      case KArgTypes::uint32:
        return encodeArray<uint32_t>(val);
      case KArgTypes::uint64:
        return encodeArray<uint64_t>(val);
      case KArgTypes::float32:
        return encodeArray<float>(val);
      case KArgTypes::float64:
        return encodeArray<double>(val);
      case KArgTypes::timestamp:
        return vec_encode_iter<KTimestamp>(
            val, [this](KTimestamp &v) { encodeTimestamp(v); });
      case KArgTypes::duration:
        return vec_encode_iter<KDuration>(
            val, [this](KDuration &v) { encodeDuration(v); });
      case KArgTypes::string:
        return vec_encode_iter<std::string>(
            val, [this](std::string &v) { encodeString(v); });
      case KArgTypes::map:
        return vec_encode_iter<KMapBase>(
            val, [this](KMapBase &v) { encodeKArgMapImpl(*v.m_map); });
      case KArgTypes::list:
        return vec_encode_iter<KListBase>(
            val, [this](KListBase &v) { encodeArgList(*v.m_list); });
      default:
        break;
      }
      return;
    } // if vector

    switch (val.m_type) {
    case KArgTypes::null:
      cbor.reserveBytes(1);
      cbor.storeByte(entazza::kCborNull);
      break;
    case KArgTypes::map:
      encodeKArgMapImpl(*val.m_value.map);
      break;
    case KArgTypes::list:
      encodeArgList(*val.m_value.list);
      break;
    case KArgTypes::string:
      cbor.add(nullptr, val.m_value.string.c_str());
      break;
    case KArgTypes::boolean:
      cbor.add(nullptr, val.m_value.boolean);
      break;
    case KArgTypes::int8:
      cbor.add(nullptr, val.as<int8_t>());
      break;
    case KArgTypes::int16:
      cbor.add(nullptr, val.as<int16_t>());
      break;
    case KArgTypes::int32:
      cbor.add(nullptr, val.as<int32_t>());
      break;
    case KArgTypes::int64:
      cbor.add(nullptr, val.as<int64_t>());
      break;
    case KArgTypes::uint8:
      cbor.add(nullptr, val.as<uint8_t>());
      break;
    case KArgTypes::uint16:
      cbor.add(nullptr, val.as<uint16_t>());
      break;
    case KArgTypes::uint32:
      cbor.add(nullptr, val.as<uint32_t>());
      break;
    case KArgTypes::uint64:
      cbor.add(nullptr, val.as<uint64_t>());
      break;
    case KArgTypes::float32:
      cbor.add(nullptr, val.as<float>());
      break;
    case KArgTypes::float64:
      cbor.add(nullptr, val.as<double>());
      break;
    case KArgTypes::custom:
      encode(KArgUtility::toArgMap(val));
      break;
    case KArgTypes::timestamp:
      encodeTimestamp(val.as<KTimestamp>());
      break;
    case KArgTypes::duration:
      encodeDuration(val.as<KDuration>());
      break;
    case KArgTypes::cfloat32:
    case KArgTypes::cfloat64:
      cbor.add(nullptr, "Error");
      break;
    }
  }

  inline CborError_t encodeKArgMapImpl(const k_arg_map_type &argMap) {
    cbor.startMap();
    for (auto const &item : argMap) {
      if (item.second.m_type == KArgTypes::null) {
        continue;
      }
#ifdef DEBUG_CODER
      std::cout << "Writing " << item.first << "\n";
#endif
      encodeArgItem(item.first.c_str(), item.second);
    }
    cbor.endMap();
    return cbor.getResult();
  }

  template <class T>
  std::shared_ptr<std::vector<T>> decodeArray(const void *bytes,
                                              uint32_t numBytes) {
    auto numElements = numBytes / sizeof(T);
    auto value = std::make_shared<std::vector<T>>(numElements);
    memcpy(value->data(), bytes, numBytes);
    return value;
  }

  template <class T>
  std::shared_ptr<std::vector<T>> decodeHomogeneousArray(size_t numElements) {
    auto v = std::make_shared<std::vector<T>>(numElements);
    for (size_t i = 0; i < numElements; i++) {
      (*v)[i] = readItem(nullptr).as<T>();
    }
    return std::move(v);
  }

  KArgVariant readItem(const char *name) {
    auto value = cbor.getNextField();
    auto tag = value.tag;
    if (tag == entazza::kCborTagTimeExt ||
        tag == entazza::kCborTagDurationExt) {
      // expect a map
      if (value.majorval != entazza::kCborMap) {
        cbor.skipField(value);
        return "Expected Map for Time/Duration";
      }
      auto numItems = cbor.getFieldValue<uint32_t>(value);
      cbor.mDataOffset += value.headerBytes; // skip map length
      uint32_t secs = 0, nano = 0;
      while (numItems-- != 0) {
        auto key = cbor.getNextField();
        auto keyValue = cbor.getFieldValue<uint8_t>(key);
        if (key.majorval == entazza::kCborPosInt && keyValue == 1) {
          cbor.mDataOffset += key.headerBytes;
          value = cbor.getNextField();
          if (value.majorval != entazza::kCborPosInt) {
            cbor.skipField(value);
            continue;
          }
          secs = cbor.getFieldValue<uint32_t>(value);
          cbor.mDataOffset += value.headerBytes;
        } else if (key.majorval == entazza::kCborNegInt &&
                   keyValue == uint8_t(-1 + 9)) {
          cbor.mDataOffset += key.headerBytes;
          value = cbor.getNextField();
          if (value.majorval != kCborPosInt) {
            cbor.skipField(value);
            continue;
          }
          nano = cbor.getFieldValue<uint32_t>(value);
          cbor.mDataOffset += value.headerBytes;
        } else {
          cbor.skipField(key);
          value = cbor.getNextField();
          cbor.skipField(value);
        }
      }
      if (tag == kCborTagTimeExt) {
        return KTimestamp(
            std::chrono::nanoseconds(secs * uint64_t(1000000000) + nano));
      } else {
        return KDuration(
            std::chrono::nanoseconds(secs * uint64_t(1000000000) + nano));
      }
    }

    if (tag == kCborTagHomogeneousArray) {
      // Expect an array where every member is the same type
      if (value.majorval != kCborArray) {
        cbor.skipField(value);
        return KArgVariant();
      }

      auto numElements = cbor.getFieldValue<uint32_t>(value);
      cbor.mDataOffset += value.headerBytes;
      if (numElements == 0) {
        // No type info available
        return KArgVariant();
      }
      // First element has the type.  Check for KArgMap array types.
      auto element = cbor.getNextField();
      if (element.majorval == kCborSimple &&
          (element.minorval == 20 || element.minorval == 21)) {
        return std::move(decodeHomogeneousArray<bool>(numElements));
      } else if (element.majorval == kCborMap && element.tag == kCborTagTimeExt) {

        cbor.mDataOffset -= 3; // Back up to read tag again inside the loop
        return std::move(decodeHomogeneousArray<KTimestamp>(numElements));
      } else if (element.majorval == kCborMap &&
                 element.tag == kCborTagDurationExt) {
        cbor.mDataOffset -= 3; // Back up to read tag again inside the loop
        return std::move(decodeHomogeneousArray<KDuration>(numElements));
      } else if (element.majorval == kCborUTF8String) {
        return std::move(decodeHomogeneousArray<std::string>(numElements));
      } else {
        // Unknown type
        while (numElements--) {
          auto element = cbor.getNextField();
          cbor.skipField(element);
        }
        return KArgVariant();
      }
    }

    switch (value.majorval) {
    case kCborByteString: {
      auto len = cbor.getFieldValue<uint32_t>(value);
      cbor.mDataOffset += value.headerBytes + len;
      const void *data = (const void *)(value.p + value.headerBytes);

      switch (value.tag) {
      case kCborTagUint8:
        return std::move(decodeArray<uint8_t>(data, len));
      case kCborTagUint16:
        return std::move(decodeArray<uint16_t>(data, len));
      case kCborTagUint32:
        return std::move(decodeArray<uint32_t>(data, len));
      case kCborTagUint64:
        return std::move(decodeArray<uint64_t>(data, len));
      case kCborTagInt8:
        return std::move(decodeArray<int8_t>(data, len));
      case kCborTagInt16:
        return std::move(decodeArray<int16_t>(data, len));
      case kCborTagInt32:
        return std::move(decodeArray<int32_t>(data, len));
      case kCborTagInt64:
        return std::move(decodeArray<int64_t>(data, len));
      case kCborTagFloat32:
        return std::move(decodeArray<float>(data, len));
      case kCborTagFloat64:
        return std::move(decodeArray<double>(data, len));
      default:
        std::string s((const char *)data, len);
        return std::move(s);
      }
    }
    case kCborUTF8String: {
      auto len = cbor.getFieldValue<uint32_t>(value);
      cbor.mDataOffset += value.headerBytes + len;

      const char *cString = (const char *)(value.p + value.headerBytes);
      // eat trailing nulls if present
      while (len && cString[len - 1] == 0) {
        len--;
      }
      std::string s(cString, len);
      return std::move(s);
    }
    case kCborNegInt: {
      int64_t v = -cbor.getFieldValue<uint64_t>(value) - 1;
      cbor.mDataOffset += value.headerBytes;
      switch (value.headerBytes) {
      case 1:
        return int8_t(v);
      case 2:
        return int8_t(v);
      case 3:
        return int16_t(v);
      case 5:
        return int32_t(v);
      case 9:
        return int64_t(v);
      default:
        return int64_t(v); // likely an error
      }
      break;
    }
    case kCborPosInt: {
      uint64_t v = cbor.getFieldValue<uint64_t>(value);
      cbor.mDataOffset += value.headerBytes;
      switch (value.headerBytes) {
      case 1:
        return uint8_t(v);
      case 2:
        return uint8_t(v);
      case 3:
        return uint16_t(v);
      case 5:
        return uint32_t(v);
      case 9:
        return uint64_t(v);
      default:
        return uint64_t(v);
      }
    }
    case kCborSimple: {
      cbor.mDataOffset += value.headerBytes;
      switch (value.minorval) {
      case 20:
        return bool(false);
      case 21:
        return bool(true);
      case 24: { // Value is next byte
        auto p = cbor.mBuf + cbor.mDataOffset;
        return *(value.p + 1);
      }
      case 26: {
        uint32_t f32 = cbor.getFieldValue<uint32_t>(value);
        return *(float *)&f32;
      }
      case 27: {
        uint64_t f64 = cbor.getFieldValue<uint64_t>(value);
        return *(double *)&f64;
      }
      default: {
        return uint64_t(0); // likely error
      }
      };
    }
    case kCborMap: {
      KArgMap map;
      auto numItems = cbor.getFieldValue<uint32_t>(value);
      cbor.mDataOffset += value.headerBytes; // skip map length
      while (numItems-- != 0) {

        // Get key name string
        value = cbor.getNextField();
        auto len = cbor.getFieldValue<uint32_t>(value);
        const auto cKey =
            (const char *)cbor.mBuf + cbor.mDataOffset + value.headerBytes;
        cbor.mDataOffset += value.headerBytes + len;
        const std::string mapKey(cKey, len);

        auto mapValue = readItem(cKey);
        map[mapKey] = mapValue;
      }
      return map;
    }
    case kCborArray: {
      auto numItems = cbor.getFieldValue<uint32_t>(value);
      cbor.mDataOffset += value.headerBytes; // skip list length
      k_arg_list_ptr result = std::make_shared<k_arg_list_type>();
      for (size_t i = 0; i < numItems; i++) {
        auto item = readItem(nullptr);
        result->push_back(item);
      }
      return result;
    }
    default:
      cbor.skipField(value);
      return "Error";
      break;
    }
  }
}; // namespace entazza
} // namespace entazza
