#include <gtest/gtest.h>
#include "sbs.hpp"

#include <fstream>
#include <string>

TEST(ParseSbs, AirbornePosition) {
    auto msg = parse_sbs("MSG,3,,,4D2023,,,,,,,20625,,,36.98971,13.84146,,,0,0,0,0");

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->transmission, Transmission::AirbornePosition);
    EXPECT_EQ(msg->icao, 0x4D2023u);
    EXPECT_EQ(msg->altitude_ft, 20625);
    ASSERT_TRUE(msg->lat_deg.has_value());
    ASSERT_TRUE(msg->lon_deg.has_value());
    EXPECT_DOUBLE_EQ(*msg->lat_deg, 36.98971);
    EXPECT_DOUBLE_EQ(*msg->lon_deg, 13.84146);
    EXPECT_FALSE(msg->ground_speed_kt.has_value());
    EXPECT_FALSE(msg->callsign.has_value());
}

TEST(ParseSbs, CallsignIsTrimmed) {
    auto msg = parse_sbs("MSG,1,,,4D2023,,,,,,AMC421  ,,,,,,,,0,0,0,0");

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->transmission, Transmission::IdAndCategory);
    EXPECT_EQ(msg->callsign, "AMC421");
}

TEST(ParseSbs, AllSpacesCallsignIsEmpty) {
    auto msg = parse_sbs("MSG,1,,,4D2023,,,,,,        ,,,,,,,,0,0,0,0");

    ASSERT_TRUE(msg.has_value());
    EXPECT_FALSE(msg->callsign.has_value());
}

TEST(ParseSbs, AirborneVelocityWithNegativeVerticalRate) {
    auto msg = parse_sbs("MSG,4,,,4D2023,,,,,,,,378,157,,,-1792,,0,0,0,0");

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->transmission, Transmission::AirborneVelocity);
    ASSERT_TRUE(msg->ground_speed_kt.has_value());
    ASSERT_TRUE(msg->track_deg.has_value());
    EXPECT_DOUBLE_EQ(*msg->ground_speed_kt, 378.0);
    EXPECT_DOUBLE_EQ(*msg->track_deg, 157.0);
    EXPECT_EQ(msg->vertical_rate_fpm, -1792);
    EXPECT_FALSE(msg->altitude_ft.has_value());
}

TEST(ParseSbs, Squawk) {
    auto msg = parse_sbs("MSG,6,,,4D2023,,,,,,,,,,,,,7700,0,0,0,0");

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->squawk, "7700");
}

TEST(ParseSbs, AllCallReplyHasOnlyIcao) {
    auto msg = parse_sbs("MSG,8,,,4D2023,,,,,,,,,,,,,,,,,");

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->transmission, Transmission::AllCallReply);
    EXPECT_EQ(msg->icao, 0x4D2023u);
    EXPECT_FALSE(msg->altitude_ft.has_value());
    EXPECT_FALSE(msg->lat_deg.has_value());
    EXPECT_FALSE(msg->alert.has_value());
    EXPECT_FALSE(msg->on_ground.has_value());
}

TEST(ParseSbs, Flags) {
    auto msg = parse_sbs("MSG,3,,,4D2023,,,,,,,20625,,,,,,,0,0,0,-1");

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->alert, false);
    EXPECT_EQ(msg->emergency, false);
    EXPECT_EQ(msg->spi, false);
    EXPECT_EQ(msg->on_ground, true);
}

TEST(ParseSbs, AcceptsTransmissionOneAndEight) {
    EXPECT_TRUE(parse_sbs("MSG,1,,,4D2023,,,,,,,,,,,,,,,,,").has_value());
    EXPECT_TRUE(parse_sbs("MSG,8,,,4D2023,,,,,,,,,,,,,,,,,").has_value());
}

TEST(ParseSbs, RejectsTransmissionZeroAndNine) {
    EXPECT_FALSE(parse_sbs("MSG,0,,,4D2023,,,,,,,,,,,,,,,,,").has_value());
    EXPECT_FALSE(parse_sbs("MSG,9,,,4D2023,,,,,,,,,,,,,,,,,").has_value());
}

TEST(ParseSbs, RejectsWrongFieldCount) {
    EXPECT_FALSE(parse_sbs("MSG,3,,,4D2023").has_value());
    EXPECT_FALSE(parse_sbs("").has_value());
}

TEST(ParseSbs, RejectsNonMsgLine) {
    EXPECT_FALSE(parse_sbs("SEL,3,,,4D2023,,,,,,,,,,,,,,,,,").has_value());
}

TEST(ParseSbs, RejectsBadIcao) {
    EXPECT_FALSE(parse_sbs("MSG,8,,,4D20ZZ,,,,,,,,,,,,,,,,,").has_value());
    EXPECT_FALSE(parse_sbs("MSG,8,,,,,,,,,,,,,,,,,,,,").has_value());
}

TEST(ParseSbs, RejectsJunkInNumberField) {
    EXPECT_FALSE(parse_sbs("MSG,3,,,4D2023,,,,,,,abc,,,,,,,0,0,0,0").has_value());
    EXPECT_FALSE(parse_sbs("MSG,3,,,4D2023,,,,,,,206.5,,,,,,,0,0,0,0").has_value());
    EXPECT_FALSE(parse_sbs("MSG,3,,,4D2023,,,,,,,99999999999,,,,,,,0,0,0,0").has_value());
    EXPECT_FALSE(parse_sbs("MSG,3,,,4D2023,,,,,,,20625,,,36.98971, 13.84146,,,0,0,0,0").has_value());
}

TEST(ParseSbs, RejectsBadFlag) {
    EXPECT_FALSE(parse_sbs("MSG,3,,,4D2023,,,,,,,20625,,,,,,,0,0,0,x").has_value());
}

TEST(ParseSbsFile, EveryLineInSampleParses) {
    std::ifstream file(SBS_SAMPLE_PATH);
    ASSERT_TRUE(file.is_open()) << "could not open " << SBS_SAMPLE_PATH;

    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        ++count;
        EXPECT_TRUE(parse_sbs(line).has_value()) << "line " << count << ": " << line;
    }
    EXPECT_EQ(count, 156);
}
