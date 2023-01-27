

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/json/robust_json.h"
#include "gtest/gtest.h"

namespace testing {

    const char *json = R"(
{
  "hello": {
    "subhello": "world",
    "number": 1024,
    "number_str": "1024",
    "double": 123.456,
    "double_str": "123.456"
  },
  "arr": [
    1,
    2,
    3,
    {
      "inside_array_key": "inside_array_value"
    },
    5,
    6
  ]
})";

    TEST(RobustJsonTest, UsageBasic) {
        rapidjson::Document doc;
        doc.Parse(json);
        rapidjson::Value &root{doc};

        {
            rapidjson::robust_json robust_json{root};
            EXPECT_EQ(robust_json["hello"]["subhello"].cast<std::string_view>(), "world");
            EXPECT_FALSE(robust_json["notexist"]["subhello"]);
            EXPECT_EQ(robust_json["notexist"]["subhello"].cast<std::string_view>(), "");
        }

        {
            rapidjson::robust_json rbdoc{root};
            rapidjson::robust_json rbhello = rbdoc["hello"];
            EXPECT_EQ(rbhello["subhello"].cast<std::string_view>(), "world");
            EXPECT_EQ(rbhello["number"].as<int64_t>().value_or(-1), 1024);
            EXPECT_EQ(rbhello["number_str"].as<int64_t>().value_or(-1), 1024);
            EXPECT_EQ(rbhello["number"].as<uint64_t>().value_or(0), 1024);
            EXPECT_EQ(rbhello["number_str"].as<uint64_t>().value_or(0), 1024);
            EXPECT_EQ(rbhello["not_exist_number"].as<int64_t>().value_or(-1), -1);
            EXPECT_EQ(rbhello["not_exist_number"].as<uint64_t>().value_or(-1), -1);
            EXPECT_FLOAT_EQ(rbhello["double"].as<double>().value_or(-1.0), 123.456);
            EXPECT_FLOAT_EQ(rbhello["double_str"].as<double>().value_or(-1.0), 123.456);
        }
    }

    TEST(RobustJsonTest, UsageObject) {
        rapidjson::Document doc;
        doc.Parse(json);
        rapidjson::Value &root{doc};

        rapidjson::robust_json rbdoc{root};
        rapidjson::robust_json rbhello = rbdoc["hello"];
        std::optional<rapidjson::Value::ConstObject> const_obj_hello = rbhello.as<rapidjson::Value::ConstObject>();
        std::optional<rapidjson::Value::Object> mutable_obj_hello = rbhello.as<rapidjson::Value::Object>();
        EXPECT_TRUE(const_obj_hello.has_value());
        EXPECT_TRUE(mutable_obj_hello.has_value());
        mutable_obj_hello.value().AddMember("newkey", "newvalue", doc.GetAllocator());
        EXPECT_EQ(rbhello["newkey"].cast<std::string_view>(), "newvalue");
    }

    TEST(RobustJsonTest, UsageArray) {
        rapidjson::Document doc;
        doc.Parse(json);
        rapidjson::Value &root{doc};

        rapidjson::robust_json rbdoc{root};
        rapidjson::robust_json rbarray = rbdoc["arr"];
        rapidjson::robust_json rbarray_0 = rbarray[0];
        EXPECT_EQ(rbarray_0.as<int64_t>().value_or(-1), 1);
        EXPECT_EQ(rbdoc["not_exist_arr"][0].as<int64_t>().value_or(-1), -1);
        EXPECT_EQ(rbdoc["arr"][9999].as<int64_t>().value_or(-1), -1);
        EXPECT_EQ(rbdoc["arr"][3]["inside_array_key"].cast<std::string_view>(), "inside_array_value");

        std::optional<rapidjson::Value::ConstArray> const_array = rbarray.as<rapidjson::Value::ConstArray>();
        std::optional<rapidjson::Value::Array> mutable_array = rbarray.as<rapidjson::Value::Array>();
        EXPECT_TRUE(const_array.has_value());
        EXPECT_TRUE(mutable_array.has_value());
        size_t pre_modify_size = mutable_array.value().Size();
        mutable_array.value().PushBack(1024, doc.GetAllocator());
        EXPECT_EQ(rbarray[pre_modify_size].as<int64_t>(), 1024);

        for (rapidjson::Value &v : mutable_array.value()) {
            rapidjson::robust_json rbv{v};
            EXPECT_TRUE(rbv);
        }
    }

    TEST(RobustJsonTest, UsageCastConstArray) {
        rapidjson::Document doc;
        doc.Parse(json);
        rapidjson::Value &root{doc};
        rapidjson::robust_json rbdoc{root};
        EXPECT_GT(rbdoc["arr"].cast<rapidjson::Value::ConstArray>().Size(), 0);
        EXPECT_EQ(rbdoc["hello"].cast<rapidjson::Value::ConstArray>().Size(), 0);
    }

    TEST(RobustJsonTest, UsageRaw) {
        rapidjson::Document doc;
        doc.Parse(json);
        rapidjson::robust_json rbdoc{(rapidjson::Value &) doc};
        rapidjson::robust_json rbobj = rbdoc["hello"];

        rapidjson::Document newdoc(rapidjson::kArrayType);
        if (rbobj) {
            newdoc.PushBack(rbobj.raw(), newdoc.GetAllocator());
            EXPECT_TRUE(rbobj.raw().IsNull());
        }
        rapidjson::robust_json robust_json{(rapidjson::Value &) newdoc};
        EXPECT_EQ(robust_json[0]["subhello"].cast<std::string_view>(), "world");
    }

}  // namespace testing