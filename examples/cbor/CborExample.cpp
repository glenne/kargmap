// SPDX-License-Identifier: BSD-3-Clause
#include <iomanip>
#include <iostream>
#include <kargmap/CborSerializer.hpp>

using namespace entazza;

int main(int argc, char **argv) {
  KArgMap m;
  m.set("count", uint16_t(1234));
  m.set("name", "Glenn");

  // Echo as JSON
  std::cout << m << std::endl;

  // Allocate space for cbor serialization
  uint8_t cborBuffer[64];
  CborSerializer cborEncoder(cborBuffer, sizeof(cborBuffer));

  // Initiate encoding into the buffer
  cborEncoder.encode(m);

  auto encodingError =
      cborEncoder.getResult(); // 0 for success, non-zero insufficient space
  auto payloadBytes =
      cborEncoder.bytesSerialized(); // number of bytes serialized
  auto bytesRequired =
      cborEncoder.bytesNeeded(); // bytes needed (if insufficient space)

  std::cout << "CBOR Payload size = " << payloadBytes
            << ", error=" << cborEncoder.getResult()
            << ", bytes required = " << bytesRequired << std::endl;

  // Dump in hex
  std::cout << "hex: ";
  for (auto i = 0; i < payloadBytes; i++) {
    std::cout << std::setfill('0') << std::setw(2) << std::hex
              << int(cborBuffer[i]);
  }
  std::cout << std::endl;

  // Deserialize a CBOR binary array into a KArgMap.
  CborSerializer cborDecoder(cborBuffer, payloadBytes);
  auto m2 = cborDecoder.decode();

  // Echo as JSON
  std::cout << m2 << std::endl;

  return 0;
}