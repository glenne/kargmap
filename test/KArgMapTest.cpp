// SPDX-License-Identifier: BSD-3-Clause
#include <chrono>
#include <cstdio>

#include "kargmap/KArgMap.hpp"
#include "gtest/gtest.h"
#include <thread>

namespace entazza {
// Helpful resources for first time google test setup
// Visual Studio: http://www.bogotobogo.com/cplusplus/google_unit_test_gtest.php
// Test Setup/Teardown:
// https://github.com/google/googletest/blob/master/googletest/docs/Primer.md

// The fixture for testing class Foo.
class KArgMapTest : public ::testing::Test {
protected:
  // Objects declared here can be used by all tests in the test case for this
  // class.

  int64_t m_startEventTime;
  std::chrono::time_point<std::chrono::steady_clock> m_startTimestamp;

  // You can remove any or all of the following functions if its body
  // is empty.

  KArgMapTest() {
    // You can do set-up work for each test here.
#ifdef TEST_DEBUG
    std::cerr << "Test Construct\n";
#endif
    m_startEventTime = 0;
  }

  virtual ~KArgMapTest() {
#ifdef TEST_DEBUG
    std::cerr << "Test Destruct\n";
#endif
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
#ifdef TEST_DEBUG
    std::cerr << "Test SetUp\n";
#endif
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
#ifdef TEST_DEBUG
    std::cerr << "Test TearDown\n";
#endif
  }

  void waitForEventTime(int64_t eventTime) const {
    // When testing flick and fling gestures, we need som semblance of real time
    // to test timers timing out or kicking in.
    auto elapsed = eventTime - m_startEventTime;
    auto now = std::chrono::steady_clock::now();
    auto sleepDur =
        elapsed - std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - m_startTimestamp)
                      .count();
    if (sleepDur > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepDur));
    }
  }
};

class MyCustomType1 {
public:
  int i;
  std::string s;
  MyCustomType1(int iValue, const std::string &sValue) : i(iValue), s(sValue) {}
  MyCustomType1() : i(0), s("") {}

  static KArgMap toArgMap(const MyCustomType1 *value) {
    return KArgMap({{"i", value->i}, {"s", value->s}});
  }

  static MyCustomType1 fromArgMap(const KArgMap &map) {
    MyCustomType1 value;
    value.i = map.get("i", value.i);
    value.s = map.get("s", value.s);
    return value;
  }

  static std::string argMapTypeName() { return "Test:MyCustomType1"; }
};
static auto myCustomType1SerializeInit =
    KArgUtility::registerCustomType<MyCustomType1>();

class MyCustomType2 {
public:
  int i = 1111;
  std::string s = "mars";
};

TEST_F(KArgMapTest, sizeMethod) {
  KArgList list{0,
                KArgMap{{"test", 1}},
                KArgList{0, 1, 2},
                std::vector<int32_t>{1, 2},
                std::vector<std::string>{"a", "b", "c", "d"},
                std::vector<float_t>{1.0, 2.0, 3.0, 4.0, 5.0},
                std::vector<KTimestamp>{KTimestamp{std::chrono::seconds(0)},
                                        KTimestamp{std::chrono::seconds(0)}}};
  ASSERT_EQ(0, list[0].size());
  ASSERT_EQ(0, list[1].size());
  ASSERT_EQ(3, list[2].size());
  ASSERT_EQ(2, list[3].size());
  ASSERT_EQ(4, list[4].size());
  ASSERT_EQ(5, list[5].size());
  ASSERT_EQ(2, list[6].size());
}
TEST_F(KArgMapTest, stdMoveKArgVariant) {
  KArgVariant v1 = "there";
  KArgVariant v2 = "there";
  v1 = std::move(v2); // used to blow up if string is not blittable
}

TEST_F(KArgMapTest, assertola) {
  KArgMap m;
#if 0
    m["ulong"] = (unsigned long)(10);
#endif
#if 0
    std::vector<int32_t> v{ 1, 2, 3 };
    m["c32"] = v;  // generates assert helptext
#endif
#if 0
    // The following should generate a compile error
    unsigned long val = 10;
    m["s"] = val;

    ASSERT_EQ(size_t(10), m.get("s", size_t(0)));
#endif
}

TEST_F(KArgMapTest, containsKey) {
  KArgMap m{{"hello", "world"}, {"i32", 1234}};

  ASSERT_TRUE(m.containsKey("hello"));
  ASSERT_TRUE(m.containsKey("i32"));
}
TEST_F(KArgMapTest, listIteration) {
  KArgList list{0, 1, 2};
  int i = 0;
  for (auto &item : list) {
    ASSERT_TRUE(item.isScalar());
    ASSERT_EQ(i++, item.get(-1));
  }

  KArgList list2{KArgMap{{"test", 1}}};
  for (auto &item : list2) {
    ASSERT_TRUE(item.isMap());
    KArgMap m = KArgMap(item);
    ASSERT_EQ(1, m.get("test", -1));
  }

  KArgList list3{KArgList{"test"}, "foo", "bar"};
  for (auto &item : list3) {
    ASSERT_TRUE(item.isList());
    KArgList l(item);
    ASSERT_EQ("test", l.get(0, "error"));
    break; // only first item is an arglist
  }
}
TEST_F(KArgMapTest, constKArgGet) {
  KArgMap config{{"x", 123}};
  KArgList list{1.0};
  std::vector<int32_t> v{1, 2, 3};
  const KArgMap m{{"config", config},
                  {"list", list},
                  {"v", std::move(v)},
                  {"i", 1},
                  {"s", "hello"}};

  auto const_config = m.get("config", KArgMap());
  ASSERT_EQ(123, const_config.get("x", -1));

  auto clist = m.get("list", KArgList());
  ASSERT_EQ(1.0, list[0].get(-1));

  auto cv = m.get<std::vector<int32_t>>("v");
  ASSERT_EQ(2, cv->at(1));

  auto ci = m.get("i", -1);
  ASSERT_EQ(1, ci);

  auto cstr = m.get("s", "fail");
  ASSERT_EQ("hello", cstr);

  const KArgList kal{config, list, std::vector<int32_t>{1, 2, 3}, 1, "hello"};

  const_config = kal.get(0, KArgMap());
  ASSERT_EQ(123, const_config.get("x", -1));

  clist = kal.get(1, KArgList());
  ASSERT_EQ(1.0, list[0].get(-1));

  // Read vector from list not yet supported
  //    cv = kal.get(2,v);
  //    ASSERT_EQ(2, cv->at(1));

  ci = kal.get(3, -1);
  ASSERT_EQ(1, ci);

  cstr = kal.get(4, "fail");
  ASSERT_EQ("hello", cstr);
}

TEST_F(KArgMapTest, assignSelf) {
  KArgMap m;
  m.set("x", "y");
  m = m;
  ASSERT_EQ("y", m.get("x", ""));
  m = m.get("z", m);
  ASSERT_EQ("y", m.get("x", ""));
}

TEST_F(KArgMapTest, boolOperations) {
  KArgMap m;
  m.set("bf", false);
  m.set("bt", true);
  m.set("bfn", 0);
  m.set("btn", 100);

  m.set("bfs", "false");
  m.set("bts", "true");
  m.set("bfs", "False");
  m.set("bts", "True");
  m.set("bfno", "no");
  m.set("btyes", "Yes");

  m.set("junk", "junk");
  m.set("tjunk", "truejunk");
  m.set("bfns", "0");
  m.set("btns", "200");

  EXPECT_FALSE(m.get("bf", true));
  EXPECT_TRUE(m.get("bt", false));
  EXPECT_FALSE(m.get("bfn", true));
  EXPECT_TRUE(m.get("btn", false));

  EXPECT_FALSE(m.get("bfs", true));
  EXPECT_TRUE(m.get("bts", false));
  EXPECT_FALSE(m.get("bfs", true));
  EXPECT_TRUE(m.get("bts", false));
  EXPECT_FALSE(m.get("bfno", true));
  EXPECT_TRUE(m.get("btyes", false));

  EXPECT_FALSE(m.get("junk", false));  // default is used
  EXPECT_TRUE(m.get("junk", true));    // default is used
  EXPECT_FALSE(m.get("tjunk", false)); // default is used
  EXPECT_TRUE(m.get("tjunk", true));   // default is used
  EXPECT_FALSE(m.get("bfns", true));
  EXPECT_FALSE(m.get("bfts", false));
}

TEST_F(KArgMapTest, setWithMove) {
  KArgMap m;
  std::string s1("hello");
  std::string s2("hello");
  m.set("test", std::move(s1));
  m.set("test", std::move(s2));
}

TEST_F(KArgMapTest, k_type_info) {
  KArgMap m;
  MyCustomType1 mt(12345, "world");
  auto ptr = std::make_shared<KArgCustomType<MyCustomType1>>(mt);
  auto code = KArgMapInternal::k_type_info<int_fast32_t>::type_code;
  ASSERT_TRUE(KArgTypes::int32 == code || KArgTypes::int64 == code);
  code = KArgMapInternal::k_type_info<KArgCustomType<MyCustomType1>>::type_code;
  ASSERT_EQ(KArgTypes::custom, code);
  code = KArgMapInternal::k_type_info<
      std::shared_ptr<KArgCustomType<MyCustomType1>>>::type_code;
  ASSERT_EQ(KArgTypes::custom, code);
  m.setCustomType("mytype", mt);
  //    m["hello"] = KArgCustomType<MyCustomType1>(mt);
}

TEST_F(KArgMapTest, customGetConversion) {
  // Verify that a type stored as an ArgMap can be retrieved
  // as a custom type.
  KArgMap m;
  MyCustomType1 cus1(13, "thirteen");
  m.set("ct", MyCustomType1::toArgMap(&cus1));

  auto cus2 = m.getCustomType<MyCustomType1>("ct");
  EXPECT_EQ(13, cus2.i);
  EXPECT_EQ("thirteen", cus2.s);

  // retrieval should still work if the type code is not present
  KArgMap m2 = MyCustomType1::toArgMap(&cus1);
  m2.erase(KArgUtility::customKeyName());
  m["ct2"] = m2;

  auto cus3 = m.getCustomType<MyCustomType1>("ct2");
  EXPECT_EQ(13, cus2.i);
  EXPECT_EQ("thirteen", cus2.s);
}

TEST_F(KArgMapTest, vectorOfArgMapToJson) {
  auto v0 = new std::vector<KArgMap>();
  auto v = std::shared_ptr<std::vector<KArgMap>>(v0);

  KArgMap map;
  map.set("i32", 1234);
  v->push_back(map);

  KArgMap map2;
  map2.set("i32", 4321);
  v->push_back(map2);
  //    KArgMapInternal::storage_type<std::shared_ptr<std::vector<int>>>::type
  //    foo; auto tc = KArgMapInternal::k_type_info<int>::type_code; auto tc2 =
  //    KArgMapInternal::k_type_info<std::shared_ptr<std::vector<KArgMap>>>::type_code;
  //    auto t =
  //    KArgMapInternal::k_type_info<std::shared_ptr<std::vector<int>>>::type_code;
  //
  //    map2.set("f", foo);
  KArgMap map3;
  map3.set("v", v);

  ASSERT_EQ("{\"v\":[{\"i32\":1234},{\"i32\":4321}]}", map3.to_string());
}

TEST_F(KArgMapTest, replaceElement) {
  // This tests the operator= in KArgVariant
  KArgMap m{{"x", 12345}};
  m.set("x", "hello");
}

TEST_F(KArgMapTest, setByPath) {
  KArgMap m2;
  m2.set("abc|2|x", 12345);
  ASSERT_EQ(12345, m2.get("abc|2|x", -1));

  KArgMap m;
  m.set("abc|xyz", 12345); // create
  auto m1 = m.get("abc", KArgMap());
  ASSERT_EQ(1, m1.size());
  auto v = m1.get("xyz", -1);
  ASSERT_EQ(12345, v);

  // container already exists
  m.set("abc|xyz", 54321); // replace
  ASSERT_EQ(54321, m1.get("xyz", -1));

  m.set("abc|xyz|def", 112233); // add child
  ASSERT_EQ(54321, m.get("abc|xyz", -1));
  ASSERT_EQ(112233, m.get("abc|xyz|def", -1));

  m["qqq"] = 123; // set
  ASSERT_EQ(123, m.get("qqq", -1));
  m.set("qqq", 456); // replace
  ASSERT_EQ(456, m.get("qqq", -1));
}

TEST_F(KArgMapTest, syntacticSugar) {
  KArgMap m;
  KArgList l;
  m.set("l", l);
  // Fails to compile
  // auto x = m.get<KArgList>("l");  //FIXME
  // OK though
  auto x2 = m.get<KArgList>("l", KArgList());
}

TEST_F(KArgMapTest, customType) {
  KArgMap m;
  MyCustomType1 mt(12345, "world");
  m.setCustomType("mytype", mt);

  auto myType = m.getCustomType("mytype", MyCustomType1());
  ASSERT_EQ(12345, myType.i);
  ASSERT_EQ("world", myType.s);

  // Type does not match.  Return default
  auto myType2 = m.getCustomType("mytype", MyCustomType2());
  ASSERT_EQ(1111, myType2.i);
  ASSERT_EQ("mars", myType2.s);

  // Test storing a shared_ptr custom type
  auto sh = std::make_shared<MyCustomType1>(54321, "shared");
  ASSERT_EQ(1, sh.use_count());
  m.setCustomType("sh", sh); // ref count bump
  ASSERT_EQ(2, sh.use_count());

  auto myType3 = m.getCustomType<std::shared_ptr<MyCustomType1>>("sh-notfound");
  ASSERT_FALSE(bool(myType3)); // nullptr check - s/b null

  myType3 = m.getCustomType<std::shared_ptr<MyCustomType1>>("sh");
  ASSERT_EQ(true, bool(myType3)); // nullptr check
  ASSERT_EQ(54321, myType3->i);
  ASSERT_EQ("shared", myType3->s);
  ASSERT_EQ(3, sh.use_count());

  m.clear(); // ref count decrement
  ASSERT_EQ(2, sh.use_count());

  // Test iterating a custom type and using getCustomType
  m.setCustomType("mt", mt);
  m.set("x", -1);
  for (auto kv : m) {
    auto v = kv.second.getCustomType<MyCustomType1>();
    ASSERT_EQ(kv.first == "mt" ? 12345 : 0, v.i);
  }

#if 0
    // future
    m["hello"] = KArgCustomType<MyCustomType1>(mt);
    auto myType4 = m.getCustomType<MyCustomType1>("hello");
    ASSERT_EQ(12345, myType4.i);
    ASSERT_EQ("world", myType4.s);
#endif

  KArgList list;
  list.addCustomType(mt);
  auto myType5 = list.getCustomType(0, MyCustomType1());

  ASSERT_TRUE(list[0].isCustom());
  ASSERT_EQ(12345, myType5.i);
  ASSERT_EQ("world", myType5.s);

  ASSERT_EQ(2, sh.use_count());
  list.addCustomType(sh);
  ASSERT_EQ(3, sh.use_count());
  auto myType6 = list.getCustomType<std::shared_ptr<MyCustomType1>>(1);
  ASSERT_EQ(true, bool(myType6)); // nullptr check
  ASSERT_EQ(54321, myType6->i);
  ASSERT_EQ("shared", myType6->s);
  ASSERT_EQ(4, sh.use_count());
  list.clear();
  ASSERT_EQ(3, sh.use_count());
}

TEST_F(KArgMapTest, listAddRemove) {
  KArgList list1{0, "hi"};
  ASSERT_EQ(2, list1.size());
  KArgList list2{1, "there"};
  list1.add(list2);
  ASSERT_EQ(4, list1.size());
  ASSERT_EQ("hi", list1.get(1, ""));
  ASSERT_EQ("there", list1.get(3, ""));

  ASSERT_EQ(1, list1.get(2, -1));
  list1.removeAt(2);
  ASSERT_EQ("there", list1.get(2, ""));
  list1.removeAt(2); // end of list
  ASSERT_EQ(2, list1.size());
  ASSERT_EQ("hi", list1.get(1, ""));
}
TEST_F(KArgMapTest, setString) {
  KArgMap m;
  m.set("s", "2.5");
  m.set("s2", "a longer string which will need some malloc space to store it");
  ASSERT_EQ("a longer string which will need some malloc space to store it",
            m.get("s2", ""));
  m.set("s2",
        "yet a longer string which will need some malloc space to store it");
  ASSERT_EQ("yet a longer string which will need some malloc space to store it",
            m.get("s2", ""));
  std::string s2("s2");
  ASSERT_EQ("yet a longer string which will need some malloc space to store it",
            m.get(s2, ""));

  std::string foo("foo");
  m.set("s3", foo);
  ASSERT_EQ("foo", m.get("s3", ""));

  const std::string foo2("foo");
  m.set("s4", foo2);
  ASSERT_EQ("foo", m.get("s4", ""));
}

TEST_F(KArgMapTest, limits) {
  KArgMap m;
  m["v"] = 1000000;
  auto v8 = m.get<int8_t>("v", -1);
  ASSERT_EQ(-1, v8);
  auto v16 = m.get<int16_t>("v", -1);
  ASSERT_EQ(-1, v16);
  auto v32 = m.get<int32_t>("v", -1);
  ASSERT_EQ(1000000, v32);

  m["v"] = -1000000;
  auto u32 = m.get<uint32_t>("v", 1);
  ASSERT_EQ(1, u32);
}

TEST_F(KArgMapTest, timestamp) {
  KArgMap m;
  KTimestamp t(std::chrono::milliseconds(1125));
  m.set("t", t);
  m.set("t", t); // test overwrite of existing element
  m.set("x", 2.5);
  m.set("s", "2.5");
  auto t2 = m.get("t", KTimestamp());
  ASSERT_NE(t, KTimestamp());
  ASSERT_EQ(t, t2);
  ASSERT_EQ(1, m.get<int32_t>("t", -1));
  ASSERT_EQ(1.125, m.get<double>("t", -1.0));

  auto t3 = m.get("x", KTimestamp());
  std::chrono::duration<double, std::milli> ts_ms = t3.time_since_epoch();
  ASSERT_EQ(2500, ts_ms.count());

  auto t4 = m.get("s", KTimestamp());
  ts_ms = t4.time_since_epoch();
  ASSERT_EQ(2500, ts_ms.count());

  KArgMap m2;
  m2.set("t", t);
  auto json = m2.to_string();
  ASSERT_EQ("{\"t\":1.125}", json);

  KArgMap m3{{"list", KArgList{t, t4}}};
  json = m3.to_string();
  ASSERT_EQ("{\"list\":[1.125,2.5]}", json);

  const KTimestamp ct(std::chrono::milliseconds(1125));
  KTimestamp x(std::chrono::system_clock::now());
  m3.set("ct", ct);
  ASSERT_EQ(1.125, m3.get<double>("ct", -1.0));

  const auto m4 = m3;
  KTimestamp ct1 = m4.get("ct", KTimestamp());
  ts_ms = ct1.time_since_epoch();
  ASSERT_EQ(1125, ts_ms.count());

  m.set("ts", "12345.125"); // from string
  KTimestamp ct2 = m.get("ts", KTimestamp());
  ts_ms = ct2.time_since_epoch();
  ASSERT_EQ(12345125, ts_ms.count());
}

TEST_F(KArgMapTest, duration) {
  KArgMap m;
  KDuration dur(std::chrono::milliseconds(1125));
  m.set("d", dur);
  m.set("d", dur); // test overwrite of existing element
  m.set("x", 2.5);
  m.set("s", "2.5");
  auto dur2 = m.get("d", KDuration());
  ASSERT_EQ(dur, dur2);
  ASSERT_EQ(1, m.get<int32_t>("d", -1));
  ASSERT_EQ(1.125, m.get<double>("d", -1.0));
  auto t3 = m.get("x", KDuration());
  std::chrono::duration<double, std::milli> ts_ms = t3;
  ASSERT_EQ(2500, ts_ms.count());

  auto t4 = m.get("s", KDuration());
  ts_ms = t4;
  ASSERT_EQ(2500, ts_ms.count());

  KArgMap m2;
  m2.set("d", dur);
  auto json = m2.to_string();
  ASSERT_EQ("{\"d\":1.125}", json);

  KArgMap m3{{"list", KArgList{dur, t4}}};
  json = m3.to_string();
  ASSERT_EQ("{\"list\":[1.125,2.5]}", json);

  const KDuration ct(std::chrono::milliseconds(1125));
  m3.set("ct", ct);
  ASSERT_EQ(1.125, m3.get<double>("ct", -1.0));

  const auto m4 = m3;
  KDuration ct1 = m4.get("ct", KDuration());
  ts_ms = ct1;
  ASSERT_EQ(1125, ts_ms.count());

  m.set("ts", "12345.125"); // from string
  KDuration ct2 = m.get("ts", KDuration());
  ts_ms = ct2;
  ASSERT_EQ(12345125, ts_ms.count());
}

TEST_F(KArgMapTest, diffVectorTypes) {
  KArgMap argMap;
  argMap.set("boola", std::vector<bool>{true, false});
  auto sp = std::make_shared<std::vector<bool>>();
  auto boola = argMap.get<std::vector<bool>>("boola", sp);
  ASSERT_EQ(2, boola->size()); // cannot get bool[] as bool[]
  ASSERT_TRUE(boola->at(0));
  auto inta = argMap.get<std::vector<int16_t>>("boola");
  ASSERT_EQ(0, inta->size()); // cannot get int[] as bool[]
}

TEST_F(KArgMapTest, stringGet) {
  KArgMap m;
  m.set("abc", "def");
  ASSERT_EQ("def", m.get("abc", ""));
  ASSERT_EQ("def", m.get<std::string>("abc", ""));
  std::string s;
  ASSERT_EQ("def", m.get("abc", s));

  const KArgMap mc = m;
  ASSERT_EQ("def", m.get("abc", ""));
  ASSERT_EQ("def", m.get("abc", s));

  // Should be compiler error
  // ASSERT_EQ("def", m.get<std::string>("abc"));
}

TEST_F(KArgMapTest, typeConvert) {
  KArgMap m{{"i16", int16_t(16)},  {"i0", 0},      {"i32", 32},
            {"u32", uint32_t(32)}, {"f32", 30.0f}, {"f64", 31.0},
            {"bf", false},         {"bt", true}};
  ASSERT_EQ(16, m.get<int16_t>("i16", -1)); // same type
  ASSERT_TRUE(m.get<bool>("i16", false));   // dest bool
  ASSERT_FALSE(m.get<bool>("i0", true));    // dest bool
  ASSERT_EQ(0, m.get<int32_t>("bf", 1));    // src bool
  ASSERT_EQ(1, m.get<int32_t>("bt", 0));    // src bool

  ASSERT_EQ(16, m.get<int32_t>("i16", -1));  // same sign
  ASSERT_EQ(32, m.get<uint16_t>("u32", -1)); // same sign

  ASSERT_EQ(32, m.get<uint16_t>("i32", -1)); // signed source, unsiged dest
  ASSERT_EQ(32, m.get<float>("u32", 1));     // unsigned source, float dest
  ASSERT_EQ(32, m.get<double>("u32", 1));    // unsigned source, float dest
  ASSERT_EQ(32, m.get<int16_t>("u32", 1));   // unsigned source, siged dest
}

TEST_F(KArgMapTest, rawVectorAdd) {
  std::vector<int32_t> v0({11, 22, 33});
  KArgMap m;
  // m.set("v", v0);  // should generate compiler error
  m.set("v", std::move(v0));
  auto def = std::make_shared<std::vector<int32_t>>();
  auto v1 = m.get<std::vector<int32_t>>("v", def);
  ASSERT_EQ(11, (v1->at(0)));
  ASSERT_EQ(22, (v1->at(1)));
  ASSERT_EQ(33, (v1->at(2)));
}

TEST_F(KArgMapTest, vectorToJson) {
  auto v0 = new std::vector<int32_t>({1, 2, 3});
  auto v = std::shared_ptr<std::vector<int32_t>>(v0);

  KArgMap map;
  map.set("v", v);

  auto v1 = map.get<std::vector<int32_t>>("v");
  ASSERT_EQ(1, (*v1)[0]);
  ASSERT_EQ(2, (*v1)[1]);

  ASSERT_EQ("{\"v\":[1,2,3]}", map.to_string());

  v1 = map.get<std::vector<int32_t>>("v");
  ASSERT_EQ(1, (*v1)[0]);
  ASSERT_EQ(2, (*v1)[1]);

  ASSERT_EQ("{\"v\":[1,2,3]}", map.to_string());
}

TEST_F(KArgMapTest, vectorOfArgListToJson) {}

TEST_F(KArgMapTest, valueGet) {
  KArgMap map;
  map.set("value", 123456);

  KArgMap map2;
  map2.set("i32", map);

  ASSERT_EQ(123456, map2.get("i32", -1));

  // double descend to get value
  KArgMap map3;
  map3.set("value", map);
  KArgMap map4;
  map4.set("i32", map3);
  ASSERT_EQ(123456, map2.get("i32", -1));

  // cause descend to not find a match
  map.set("value", KArgMap());
  ASSERT_EQ(-1, map2.get("i32", -1));
}

TEST_F(KArgMapTest, helloWorld) {
  KArgMap map;
  map["s"] = "hello world";
  ASSERT_EQ("hello world", map.get("s", "err"));

  map.set("s2", "hi");
  ASSERT_EQ("hi", map.get("s2", "err"));
}

TEST_F(KArgMapTest, roundTrip) {
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

  ASSERT_EQ(-10, map.get<int8_t>("i8", -100));
  ASSERT_EQ(-10000, map.get<int16_t>("i16", -100));
  ASSERT_EQ(-100000, map.get<int32_t>("i32", -100));
  ASSERT_EQ(-10000000000, map.get<int64_t>("i64", -100));
  ASSERT_EQ(10, map.get<uint8_t>("ui8", 100));
  ASSERT_EQ(10000, map.get<uint16_t>("ui16", 100));
  ASSERT_EQ(100000, map.get<uint32_t>("ui32", 100));
  ASSERT_EQ(10000000000, map.get<uint64_t>("ui64", 100));
  ASSERT_EQ(1.25e20f, map.get<float>("f32", 100));
  ASSERT_EQ(1.25e100, map.get<double>("f64", 100));
  ASSERT_EQ("test", map.get("s", "fail"));
}

TEST_F(KArgMapTest, json) {
  KArgMap map;
  map.set("foo", "bar");
  auto json = std::string(map);
  auto json2 = map.to_string();

  ASSERT_EQ("{\"foo\":\"bar\"}", json);
  ASSERT_EQ(json, json2);
}

TEST_F(KArgMapTest, uint8_to_string) {
  KArgMap map;
  map.set("foo",uint8_t(240));
  auto json = std::string(map);
  auto json2 = map.to_string();

  ASSERT_EQ("{\"foo\":240}", json);
  ASSERT_EQ(json, json2);
}

TEST_F(KArgMapTest, json_stream) {
  KArgMap map;
  map.set("foo", "bar");
  std::stringstream s;
  s << map;
  ASSERT_EQ("{\"foo\":\"bar\"}", s.str());
}

TEST_F(KArgMapTest, mapget) {
  KArgMap map;
  KArgMap map2;
  ASSERT_EQ(1, map2.use_count());
  map.set("m2", map2);
  ASSERT_EQ(2, map2.use_count());
  KArgMap map3 = map.get("m2", KArgMap());
  ASSERT_EQ(3, map2.use_count());
}

TEST_F(KArgMapTest, nesting) {
  KArgMap map;
  KArgMap map2;
  {
    KArgMap map1 = map;
    ASSERT_EQ(2, map.use_count());
  }
  ASSERT_EQ(1, map2.use_count());

  map2.set("i32", 12345);
  ASSERT_EQ(1, map2.use_count());
  map.set("m2", map2);
  ASSERT_EQ(2, map2.use_count());
  KArgMap map3 = map.get("m2", KArgMap());
  ASSERT_EQ(3, map2.use_count());
  ASSERT_EQ(12345, map3.get("i32", 0));

  ASSERT_EQ("{\"m2\":{\"i32\":12345}}", map.to_string());

  KArgList list;
  list.push_back(1);
  list.push_back("hi");

  // Test list in map
  map.set("list", list);
  ASSERT_EQ(2, list.use_count());
  auto list2 = map.get("list", KArgList());
  ASSERT_EQ(3, list.use_count());
  ASSERT_EQ("hi", list2.get<std::string>(1, "error"));

  // Test decrease in references
  ASSERT_EQ(3, list.use_count());
  map = KArgMap();
  ASSERT_EQ(2, list.use_count());

  // test map in list
  map.set("i32", 4321);
  ASSERT_EQ(1, map.use_count());
  list.push_back(map);
  ASSERT_EQ(2, map.use_count());
}

TEST_F(KArgMapTest, argList) {
  KArgList list;
  ASSERT_EQ(0, list.size());
  list.push_back(1);
  ASSERT_EQ(1, list.size());
  list.push_back("hi");
  ASSERT_EQ(2, list.size());

  ASSERT_EQ(2, list.size());
  ASSERT_EQ(1, list.get<int32_t>(0, 0));
  ASSERT_EQ(1, list.get(0, 0));

  ASSERT_EQ("hi", list.get<std::string>(1, "error"));
  ASSERT_EQ("hi", list.get(1, "error"));

  std::stringstream s;
  s << list;
  ASSERT_EQ("[1,\"hi\"]", s.str());

  list.set(1, "xyz");
  ASSERT_EQ("xyz", list.get(1, ""));

  list[1] = "pdq";
  ASSERT_EQ("pdq", list.get(1, ""));

  // works but visual studio thinks it's fatal
  // EXPECT_THROW(list[10] = 1.0, std::exception);
}

TEST_F(KArgMapTest, fromString) {
  KArgMap map;
  map.set("i8", "-10");
  map.set("i16", "-10000");
  map.set("i32", "-100000");
  map.set("i64", "-10000000000");
  map.set("ui8", "10");
  map.set("ui16", "10000");
  map.set("ui32", "100000");
  map.set("ui64", "10000000000");
  map.set("f32", "1.25e20");
  map.set("f64", "1.25e100");
  ASSERT_EQ(-10, map.get<int8_t>("i8", -100));
  ASSERT_EQ(-10000, map.get<int16_t>("i16", -100));
  ASSERT_EQ(-100000, map.get<int32_t>("i32", -100));
  ASSERT_EQ(-10000000000, map.get<int64_t>("i64", -100));
  ASSERT_EQ(10, map.get<uint8_t>("ui8", 100));
  ASSERT_EQ(10000, map.get<uint16_t>("ui16", 100));
  ASSERT_EQ(100000, map.get<uint32_t>("ui32", 100));
  ASSERT_EQ(10000000000, map.get<uint64_t>("ui64", 100));
  ASSERT_EQ(1.25e20f, map.get<float>("f32", 100));
  ASSERT_EQ(1.25e100, map.get<double>("f64", 100));

  // conversion float to [u]int
  map.set("f1", "29.3");
  ASSERT_EQ(29, map.get<int8_t>("f1", -100));
  ASSERT_EQ(29, map.get<int16_t>("f1", -100));
  ASSERT_EQ(29, map.get<int32_t>("f1", -100));
  ASSERT_EQ(29, map.get<int64_t>("f1", -100));
  ASSERT_EQ(29, map.get<uint8_t>("f1", 100));
  ASSERT_EQ(29, map.get<uint16_t>("f1", 100));
  ASSERT_EQ(29, map.get<uint32_t>("f1", 100));
  ASSERT_EQ(29, map.get<uint64_t>("f1", 100));

  // conversions that fail
  map.set("f1", "junk");
  ASSERT_EQ(-100, map.get<int8_t>("f1", -100));
  ASSERT_EQ(-100, map.get<int16_t>("f1", -100));
  ASSERT_EQ(-100, map.get<int32_t>("f1", -100));
  ASSERT_EQ(-100, map.get<int64_t>("f1", -100));
  ASSERT_EQ(100, map.get<uint8_t>("f1", 100));
  ASSERT_EQ(100, map.get<uint16_t>("f1", 100));
  ASSERT_EQ(100, map.get<uint32_t>("f1", 100));
  ASSERT_EQ(100, map.get<uint64_t>("f1", 100));
}

TEST_F(KArgMapTest, toString) {
  KArgMap map;
  map.set("ui64", uint64_t(10000000000));
  ASSERT_EQ("10000000000", map.get("ui64", "100"));

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

  ASSERT_EQ("-10", map.get("i8", "-100"));
  ASSERT_EQ("-10000", map.get("i16", "-100"));
  ASSERT_EQ("-100000", map.get("i32", "-100"));
  ASSERT_EQ("-10000000000", map.get("i64", "-100"));
  ASSERT_EQ("10", map.get("ui8", "100"));
  ASSERT_EQ("10000", map.get("ui16", "100"));
  ASSERT_EQ("100000", map.get("ui32", "100"));
  ASSERT_EQ("10000000000", map.get("ui64", "100"));
  ASSERT_EQ("1.25e+20", map.get("f32", "100"));
  ASSERT_EQ("1.25e+100", map.get("f64", "100"));
}

TEST_F(KArgMapTest, conversions) {}

// http://stackoverflow.com/questions/19616586/shared-ptrt-to-shared-ptrt-const-and-vectort-to-vectort-const
TEST_F(KArgMapTest, constValues) {
  auto v0 = new std::vector<int32_t>({1, 2, 3});
  auto v = std::shared_ptr<std::vector<int32_t>>(v0);
  std::shared_ptr<const std::vector<int32_t>> cv = v;

  KArgMap map;
  ASSERT_EQ(1, map.use_count());
  map.set("v", v);
  //    map.set("abc", cv);  // expect compile error

  const auto cmap = map;
  ASSERT_EQ(2, map.use_count());
  //    cmap.set("abc", 10);  // expect compile error

  const auto cmap2 = map;
  ASSERT_EQ(3, map.use_count());
  ASSERT_EQ(3, cmap.use_count());
  ASSERT_EQ(3, cmap2.use_count());
  //    cmap2.set("abc", 10);  // expect compile error

  v = map.get("v", v); // OK
  // get non-const from const - invalid
  // v = cmap.get("v", v); // compile error

  // next OK
  //   std::shared_ptr<const std::vector<int32_t>> cv2 = cmap.get("v", cv);

  // Extract a std::vector from a const KArgList should result in a const
  // std::vector
  const KArgList clist = {1, 2, std::vector<int>{3, 4, 5}};
  auto cv2 = clist.get<std::shared_ptr<std::vector<int>>>(
      2, std::shared_ptr<std::vector<int>>());
  ASSERT_EQ(3, cv2->at(0));
#if 0
    constexpr bool const_type_list = std::is_same<decltype(cv2), std::shared_ptr<const std::vector<int>>>::value;
    EXPECT_TRUE(const_type_list);
//    cv2->at(0) = 1;  // Should fail to compile!

    // Extract a std::vector from a const KArgMap should result in a const std::vector

    const KArgMap cmap3  { {"a",1}, {"b", 2 }, {"c", v} };
     auto cv3 = cmap3.get<std::shared_ptr<std::vector<int32_t>>>("c");
     constexpr bool const_type_map = std::is_same<decltype(cv3), std::shared_ptr<const std::vector<int>>>::value;
     ASSERT_TRUE(const_type_map);
#endif
  //    cv3->at(0) = 1;  // Should fail to compile!
}

TEST_F(KArgMapTest, time) {}

// README.md syntax verify
KArgMap createSettings() {
  KArgMap argMap{{"name", "Captain Kirk"}};     // initializer list
  argMap.set("occupation", "Starship Captain"); // set operator

  argMap["age"] = 85; // array operator with assignment
  argMap["warpFactor"] = 7.8;

  std::cout << argMap; // log as json
  return argMap;       // smart pointer copy
}

void applySettings(const KArgMap settings) {
  auto name = settings.get("name", "unknown");
  auto job = settings.get("occupation", "unknown");

  int32_t age = settings.get("age", 0);
  double warpFactor = settings.get("warpFactor", 0.0);

  auto json = settings.to_string();
}

TEST_F(KArgMapTest, initializerList) {
  KArgMap m(
      {{"abc", "abc"}, {"i32", 1234}, {"map", KArgMap{{"x", 1}, {"y", 2}}}});
  ASSERT_EQ("abc", m.get("abc", "error"));
  ASSERT_EQ(1234, m.get("i32", -1));

  KArgList list({"abc", 1234});
  ASSERT_EQ("abc", list.get(0, "error"));
  ASSERT_EQ(1234, list.get(1, -1));

  KArgMap map = m.get("map", KArgMap());
  ASSERT_EQ(1, map.get("x", -1));
  ASSERT_EQ(2, map.get("y", -1));
}

TEST_F(KArgMapTest, deepAdd) {}

TEST_F(KArgMapTest, deepClone) {
  KArgMap m;
  KArgMap m2;
  m.set("m2", m2);
  m.set("abc", "abc");
  m2.set("xyz", "xyz");
  KArgList list({"abc", 1234});
  m2.set("list", list);
  ASSERT_EQ(1, m.use_count());
  ASSERT_EQ(2, m2.use_count());
  ASSERT_EQ(2, m2.use_count());

  KArgMap mclone = m.deepClone();
  ASSERT_EQ(1, mclone.use_count());
  auto m2copy = mclone.get("m2", KArgMap());
  auto list2copy = m2copy.get("list", KArgList());

  ASSERT_EQ(2, m2copy.use_count());
  ASSERT_EQ(2, list2copy.use_count());
  ASSERT_EQ(1, m.use_count());
  ASSERT_EQ(2, m2.use_count());

  ASSERT_EQ("xyz", m2copy.get("xyz", "error"));
  ASSERT_EQ("abc", list2copy.get(0, "error"));
}

TEST_F(KArgMapTest, duplicate) {}

TEST_F(KArgMapTest, FLexGet) {
  KArgMap m;
  m.set("range", -30);              // store as integer in version1
  auto r1 = m.get<int>("range", 0); // get as an integer
  ASSERT_EQ(-30, r1);

  // In version 2 it changed to be stored as a double.  v1 code continues to
  // work. Return type is imputed from get usage hints
  auto r2 = m.get("range", 0.0);       // double because default is a double
  auto r3 = m.get<double>("range", 0); // double because get<T> specified
  auto r4 = m.get("range", double(0)); // double because default value is double
  auto r5 = m.get("range", "error");   // get as a string

  ASSERT_EQ(-30.0, r2);
  ASSERT_EQ(-30.0, r3);
  ASSERT_EQ(-30.0, r4);
  ASSERT_EQ("-30", r5);
}

TEST_F(KArgMapTest, README_MD) {
  auto settings = createSettings();
  applySettings(settings);
  ASSERT_EQ("Captain Kirk", settings.get("name", "error"));
  ASSERT_EQ("Starship Captain", settings.get("occupation", "error"));

  ASSERT_EQ(85, settings.get("age", -1));
  ASSERT_EQ(7.8, settings.get("warpFactor", -1.0));

  // In version1 we store 'range' as a plain double.
  KArgMap version1;
  version1.set("range", -30.0);
  ASSERT_EQ("{\"range\":-30}", version1.to_string());

  // In version2 it was realized that units meta data was needed.
  // 'value' field is used for range 'value' with a sibling 'units' field.
  KArgMap version2;
  KArgMap range;
  range.set("value", -30.0);
  range.set("units", "dBm");
  version2.set("range",
               range); // range is now stored as KArgMap key/value pairs
  ASSERT_TRUE("{\"range\":{\"value\":-30, \"units\":\"dBm\"}}" ==
                  version2.to_string() ||
              "{\"range\":{\"units\":\"dBm\", \"value\":-30}}" ==
                  version2.to_string());

  // Version1 code can read a version2 encoded range without any code changes
  ASSERT_EQ(version1.get("range", 0.0), version2.get("range", 0.0));

  auto v1Range = version1.get("range", 0.0);
  auto v2Range = version2.get("range", 0.0); // old code still works for "range"
  auto v2Units = version2.get("range|units", "V"); // new code can access units

  ASSERT_EQ(-30.0, v1Range);
  ASSERT_EQ(-30.0, v2Range);
  ASSERT_EQ("dBm", v2Units);

  auto units = version2.get("range|", "V"); // new code can access units
  ASSERT_EQ("V", units);
}

TEST_F(KArgMapTest, ComplexScalar32) {
  KArgMap m;
  auto cvalue = std::complex<float>(1.0f, 2.0f);
  m["c32"] = cvalue;
  auto arg = m["c32"];
  auto c32 = m.get("c32", std::complex<float>(1.0, 2.0));
  ASSERT_EQ(1.0f, c32.real());
  ASSERT_EQ(2.0f, c32.imag());

  auto s = m.get<std::string>("c32", "err");
  ASSERT_EQ("(1,2)", s);
}

TEST_F(KArgMapTest, ComplexVector32) {
  KArgMap m;
  auto cvalue = std::vector<std::complex<float>>({{1.0f, 2.0f}, {3.0f, 5.0f}});
  m["c32"] = std::move(cvalue);
  auto arg = m["c32"];
  ASSERT_EQ(arg.getType(), KArgTypes::cfloat32);
  auto c32 = m.get<std::vector<std::complex<float>>>("c32", {});
  ASSERT_EQ(1.0f, (*c32)[0].real());
  ASSERT_EQ(2.0f, (*c32)[0].imag());

  std::stringstream s;
  s << m;
  ASSERT_EQ("{\"c32\":[\"(1,2)\",\"(3,5)\"]}", s.str());
}

TEST_F(KArgMapTest, ComplexScalar64) {
  KArgMap m;
  auto cvalue = std::complex<double>(1.0, 2.0);
  m["c64"] = cvalue;
  auto arg = m["c64"];
  auto c64 = m.get("c64", std::complex<double>(1.0, 2.0));
  ASSERT_EQ(1.0f, c64.real());
  ASSERT_EQ(2.0f, c64.imag());

  auto s = m.get<std::string>("c64", "err");
  ASSERT_EQ("(1,2)", s);
}

TEST_F(KArgMapTest, ComplexVector64) {
  KArgMap m;
  auto cvalue = std::vector<std::complex<double>>({{1.0f, 2.0f}, {3.0f, 5.0f}});
  m["c64"] = std::move(cvalue);
  auto arg = m["c64"];
  ASSERT_EQ(arg.getType(), KArgTypes::cfloat64);
  auto c64 = m.get<std::vector<std::complex<double>>>("c64", {});
  ASSERT_EQ(1.0f, (*c64)[0].real());
  ASSERT_EQ(2.0f, (*c64)[0].imag());

  std::stringstream s;
  s << m;
  ASSERT_EQ("{\"c64\":[\"(1,2)\",\"(3,5)\"]}", s.str());
}

} // namespace entazza

int main(int argc, char **argv) {
  // debug arguments such as pause --gtest_filter=KArgMapTest.string*
  bool pauseOnExit = argc > 1 && std::string(argv[1]) == "pause";
  ::testing::InitGoogleTest(&argc, argv);

  auto result = RUN_ALL_TESTS();
  if (pauseOnExit)
    getchar();
  return result;
}
