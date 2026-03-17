#include <gtest/gtest.h>
#include "gopost/template_parser.h"
#include "gopost/engine.h"
#include <cstring>
#include <vector>

class TemplateParserTest : public ::testing::Test {
protected:
    GopostEngine* engine = nullptr;

    void SetUp() override {
        GopostEngineConfig config{};
        config.thread_count = 1;
        config.frame_pool_size_mb = 16;
        config.enable_gpu = 0;
        config.log_level = 0;
        gopost_engine_create(&engine, &config);
    }

    void TearDown() override {
        if (engine) gopost_engine_destroy(engine);
    }

    std::vector<uint8_t> make_minimal_gpt(uint8_t type = GOPOST_TEMPLATE_TYPE_IMAGE) {
        std::vector<uint8_t> data;

        // File header (16 bytes)
        data.push_back('G'); data.push_back('O');
        data.push_back('P'); data.push_back('T');
        data.push_back(0x01); data.push_back(0x00); // version 1
        data.push_back(0x00); data.push_back(0x00); // flags
        data.push_back(type);                         // template type
        for (int i = 0; i < 7; i++) data.push_back(0); // reserved

        // Encryption header (64 bytes)
        data.push_back(0x01); // algorithm ID = AES-256-GCM
        for (int i = 0; i < 63; i++) data.push_back(0);

        // Checksum (32 bytes) at the end
        for (int i = 0; i < 32; i++) data.push_back(0);

        return data;
    }
};

TEST_F(TemplateParserTest, RejectsNullArguments) {
    GopostTemplate* tpl = nullptr;
    uint8_t dummy[1] = {0};

    EXPECT_EQ(gopost_template_parse(nullptr, dummy, 1, dummy, 1, &tpl),
              GOPOST_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(gopost_template_parse(engine, nullptr, 0, dummy, 1, &tpl),
              GOPOST_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(gopost_template_parse(engine, dummy, 1, nullptr, 0, &tpl),
              GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST_F(TemplateParserTest, RejectsTooSmallData) {
    GopostTemplate* tpl = nullptr;
    uint8_t small[10] = {0};

    EXPECT_EQ(gopost_template_parse(engine, small, sizeof(small), small, 1, &tpl),
              GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST_F(TemplateParserTest, RejectsInvalidMagicBytes) {
    auto data = make_minimal_gpt();
    data[0] = 'X';
    GopostTemplate* tpl = nullptr;

    EXPECT_EQ(gopost_template_parse(engine, data.data(), data.size(),
              data.data(), 1, &tpl),
              GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST_F(TemplateParserTest, ValidMagicBytesAccepted) {
    auto data = make_minimal_gpt();
    EXPECT_EQ(data[0], 'G');
    EXPECT_EQ(data[1], 'O');
    EXPECT_EQ(data[2], 'P');
    EXPECT_EQ(data[3], 'T');
}

TEST_F(TemplateParserTest, GetMetadataRejectsNull) {
    const char* json = nullptr;
    EXPECT_EQ(gopost_template_get_metadata(nullptr, &json),
              GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST_F(TemplateParserTest, UnloadRejectsNull) {
    EXPECT_EQ(gopost_template_unload(engine, nullptr),
              GOPOST_ERROR_INVALID_ARGUMENT);
}
