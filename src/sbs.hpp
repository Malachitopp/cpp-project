#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view> 


enum class Transmission : std::uint8_t {
    IdAndCategory     = 1,
    SurfacePosition   = 2,
    AirbornePosition  = 3,
    AirborneVelocity  = 4,
    SurveillanceAlt   = 5,
    SurveillanceId    = 6,
    AirToAir          = 7,
    AllCallReply      = 8,
};

struct SbsMessage {
    Transmission transmission {};
    std::uint32_t icao {};

    std::optional<std::string> callsign;
    std::optional<std::int32_t> altitude_ft;
    std::optional<double> ground_speed_kt;
    std::optional<double> track_deg;
    std::optional<double> lat_deg;
    std::optional<double> lon_deg;
    std::optional<std::int32_t> vertical_rate_fpm;
    std::optional<std::string> squawk;

    std::optional<bool> alert;
    std::optional<bool> emergency;
    std::optional<bool> spi;
    std::optional<bool> on_ground;
};


std::optional<SbsMessage> parse_sbs(std::string_view line); 
