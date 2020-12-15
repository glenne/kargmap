// SPDX-License-Identifier: BSD-3-Clause

#if !defined(CONFIG_MICROCBOR_STD_VECTOR)
#define CONFIG_MICROCBOR_STD_VECTOR
#endif

#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include <kargmap/CborSerializer.hpp>
#include <kargmap/KArgMap.hpp>

namespace entazza {

KArgMap roundTripToCbor(KArgMap map) {
  uint8_t buffer[4096];
  CborSerializer coder(buffer, sizeof(buffer));
  coder.encode(map);

  CborSerializer decoder(buffer, sizeof(buffer));
  auto m = decoder.decode();
  return m;
}

TEST(KArgMapCborTest, string_array) {
  auto s0 = std::string("s0");
  auto s1 = std::string("s1");
  KArgMap map;
  map.set("sa", std::vector<std::string>{s0, s1});

  auto map2 = roundTripToCbor(map);

  auto sp = std::make_shared<std::vector<std::string>>();
  auto sa = map2.get("sa", sp);
  ASSERT_EQ(2, sa->size());
  ASSERT_EQ(s0, sa->at(0));
  ASSERT_EQ(s1, sa->at(1));
}

TEST(KArgMapCborTest, duation_array) {
  auto t0 = KDuration(std::chrono::milliseconds(1125));
  auto t1 = KDuration(std::chrono::milliseconds(2222));
  KArgMap map;
  map.set("dura", std::vector<KDuration>{t0, t1});

  auto map2 = roundTripToCbor(map);

  auto sp = std::make_shared<std::vector<KDuration>>();
  auto dura = map2.get("dura", sp);
  ASSERT_EQ(2, dura->size());
  ASSERT_EQ(t0, dura->at(0));
  ASSERT_EQ(t1, dura->at(1));
}

TEST(KArgMapCborTest, timestamp_array) {
  auto t0 = KTimestamp(std::chrono::milliseconds(1125));
  auto t1 = KTimestamp(std::chrono::milliseconds(2222));
  KArgMap map;
  map.set("tsa", std::vector<KTimestamp>{t0, t1});

  auto map2 = roundTripToCbor(map);

  auto sp = std::make_shared<std::vector<KTimestamp>>();
  auto tsa = map2.get("tsa", sp);
  ASSERT_EQ(2, tsa->size());
  ASSERT_EQ(t0, tsa->at(0));
  ASSERT_EQ(t1, tsa->at(1));
}

TEST(KArgMapCborTest, bool_array) {
  KArgMap map;
  map.set("boola", std::vector<bool>{true, false, true});

  auto map2 = roundTripToCbor(map);

  auto sp = std::make_shared<std::vector<bool>>();
  auto boola = map2.get<std::vector<bool>>("boola", sp);
  ASSERT_EQ(3, boola->size()); // cannot get bool[] as bool[]
  ASSERT_TRUE(boola->at(0));
  ASSERT_FALSE(boola->at(1));
  ASSERT_TRUE(boola->at(2));
}

TEST(KArgMapCborTest, bool) {
  KArgMap map;
  map.set("bf", false);
  map.set("bt", true);

  auto map2 = roundTripToCbor(map);

  EXPECT_FALSE(map2.get("bf", true));
  EXPECT_TRUE(map2.get("bt", false));
}

TEST(KArgMapCborTest, timestamp) {
  KArgMap map;
  KTimestamp t(std::chrono::milliseconds(1125));
  map.set("t", t);

  auto map2 = roundTripToCbor(map);

  auto t2 = map2.get("t", KTimestamp());
  ASSERT_NE(t, KTimestamp());
  ASSERT_EQ(t, t2);
  ASSERT_EQ(1, map2.get<int32_t>("t", -1));
  ASSERT_EQ(1.125, map2.get<double>("t", -1.0));
}

TEST(KArgMapCborTest, duration) {
  KArgMap map;
  KDuration dur(std::chrono::milliseconds(1125));
  map.set("d", dur);

  auto map2 = roundTripToCbor(map);

  auto dur2 = map2.get("d", KDuration());
  ASSERT_EQ(dur, dur2);
  ASSERT_EQ(1, map2.get<int32_t>("d", -1));
  ASSERT_EQ(1.125, map2.get<double>("d", -1.0));
}

TEST(KArgMapCborTest, kargmap_child) {
  KArgMap map;
  KArgMap childMap;
  map.set("child", childMap);
  childMap.set("a", int16_t(1234));
  childMap.set("b", "itemb");

  auto map2 = roundTripToCbor(map);

  auto childMap2 = map2.get("child", KArgMap());

  ASSERT_EQ(2, childMap2.size());

  ASSERT_EQ(1234, childMap2.get("a", -1));
  ASSERT_EQ("itemb", childMap2.get("b", "fail"));
}

TEST(KArgMapCborTest, list) {
  KArgList list = {int16_t(1234), "test"};
  KArgMap map;
  map.set("list", list);

  auto map2 = roundTripToCbor(map);

  list = map2.get("list", KArgList());

  ASSERT_EQ(2, list.size());

  ASSERT_EQ(1234, list[0].get(-1));
  ASSERT_EQ("test", list[1].get("fail"));
}

TEST(KArgMapCborTest, basic_s) {
  KArgMap map;
  map.set("s", "test");

  auto map2 = roundTripToCbor(map);

  ASSERT_EQ("test", map2.get("s", "fail"));
}

#ifdef CONFIG_MICROCBOR_STD_VECTOR
template <typename T> void testVector() {
  std::vector<T> pts = {1, 2, 3, 4};
  if (std::is_signed<T>::value) {
    pts = {1, 2, T(-3), T(-4)};
  }
  KArgMap map;
  map.set("pts", std::move(pts));

  auto map2 = roundTripToCbor(map);

  auto p = map2.get<std::vector<T>>("pts");
  ASSERT_EQ(4, p->size());

  ASSERT_EQ(1, (*p)[0]);
  ASSERT_EQ(2, p->at(1));
  if (std::is_signed<T>::value) {
    ASSERT_EQ(-3, p->at(2));
    ASSERT_EQ(-4, p->at(3));
  } else {
    ASSERT_EQ(3, p->at(2));
    ASSERT_EQ(4, p->at(3));
  }
}

TEST(KArgMapCborTest, vec_int_8_t) { testVector<int8_t>(); }
TEST(KArgMapCborTest, vec_int_16_t) { testVector<int16_t>(); }
TEST(KArgMapCborTest, vec_int_32_t) { testVector<int32_t>(); }
TEST(KArgMapCborTest, vec_int_64_t) { testVector<int64_t>(); }
TEST(KArgMapCborTest, vec_uint_8_t) { testVector<uint8_t>(); }
TEST(KArgMapCborTest, vec_uint_16_t) { testVector<uint16_t>(); }
TEST(KArgMapCborTest, vec_uint_32_t) { testVector<uint32_t>(); }
TEST(KArgMapCborTest, vec_uint_64_t) { testVector<uint64_t>(); }
TEST(KArgMapCborTest, vec_float_t) { testVector<float_t>(); }
TEST(KArgMapCborTest, vec_double_t) { testVector<double_t>(); }

#endif

TEST(KArgMapCborTest, basic_f64) {
  KArgMap map;
  map.set("f64", double(1.25e100));

  auto map2 = roundTripToCbor(map);

  ASSERT_EQ(1.25e100, map2.get<double>("f64", 100));
}

TEST(KArgMapCborTest, basic_f32) {
  KArgMap map;
  map.set("f32", float(1.25e20f));

  auto map2 = roundTripToCbor(map);

  ASSERT_EQ(1.25e20f, map2.get<float>("f32", 100));
}

TEST(KArgMapCborTest, basic_i8) {
  KArgMap map;
  map.set("i8", int8_t(-10));

  auto map2 = roundTripToCbor(map);

  ASSERT_EQ(-10, map2.get<int8_t>("i8", -100));
}

TEST(KArgMapCborTest, roundTrip) {
  KArgMap map;
  map.set("i8", int8_t(-10));
  map.set("i16", int16_t(-10000));
  map.set("i32", int32_t(-100000));
  map.set("i64", int64_t(-10000000000));
  map.set("ui8", uint8_t(10));
  map.set("ui16", uint16_t(10000));
  map.set("ui32", uint32_t(100000));
  map.set("ui64", uint64_t(10000000000));
  map.set("f32", float(1.25e20f));
  map.set("f64", double(1.25e100));
  map.set("s", "test");

  auto map2 = roundTripToCbor(map);

  // std::cout << map2 << std::endl;
  ASSERT_EQ(-10, map2.get<int8_t>("i8", -100));
  ASSERT_EQ(-10000, map2.get<int16_t>("i16", -100));
  ASSERT_EQ(-100000, map2.get<int32_t>("i32", -100));
  ASSERT_EQ(-10000000000, map2.get<int64_t>("i64", -100));
  ASSERT_EQ(10, map2.get<uint8_t>("ui8", 100));
  ASSERT_EQ(10000, map2.get<uint16_t>("ui16", 100));
  ASSERT_EQ(100000, map2.get<uint32_t>("ui32", 100));
  ASSERT_EQ(10000000000, map2.get<uint64_t>("ui64", 100));
  ASSERT_EQ(1.25e20f, map2.get<float>("f32", 100));
  ASSERT_EQ(1.25e100, map2.get<double>("f64", 100));
  ASSERT_EQ("test", map2.get("s", "fail"));
}

} // namespace entazza

int main(int argc, char **argv) {
  // debug arguments such as pause --gTESTilter=KArgMapCborTest.string*
  bool pauseOnExit = argc > 1 && std::string(argv[1]) == "pause";
  ::testing::InitGoogleTest(&argc, argv);

  auto result = RUN_ALL_TESTS();
  if (pauseOnExit)
    getchar();
  return result;
}
