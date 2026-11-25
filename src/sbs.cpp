#include "sbs.hpp"

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>
//namespace can only be used in this file 
namespace {

constexpr std::size_t kFieldCount = 22;

constexpr std::size_t kMessageType = 0;
constexpr std::size_t ktransmission = 1;
constexpr std::size_t kIcao = 4;
constexpr std::size_t kCallsign = 10;
constexpr std::size_t kAltitude = 11;
constexpr std::size_t kGroundSpeed = 12;
constexpr std::size_t kTrack = 13;
constexpr std::size_t kLat = 14;
constexpr std::size_t kLon = 15;
constexpr std::size_t kVerticalRate = 16;
constexpr std::size_t kSquawk = 17;
constexpr std::size_t kAlert = 18;
constexpr std::size_t kEmergency = 19;
constexpr std::size_t kSpi = 20;
constexpr std::size_t kOnGround = 21;

std::optional<std::uint32_t> parse_hex(std::string_view text)
{
    std::uint32_t value {};
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}
std::optional<std::uint32_t> parse_trans(std::string_view text) {
    std::uint32_t value {};
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size() || 1 > value || 8 < value) {
        return std::nullopt;
    }

    return value;
}

template <typename T>
bool parse_T(std::string_view text, std::optional<T>& out) {
    if (text.empty()) {return true;}
    T value {};
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        return false;
    }
    out = value;
    return true;
}

bool parse_text(std::string_view text, std::optional<std::string>& out) {
    std::size_t last = text.find_last_not_of(' ');
    if (last == std::string_view::npos) {
        return true;
    }
    out = std::string(text.substr(0, last + 1));
    return true;
}

bool parse_flags(std::string_view text, std::optional<bool>& out) {
    if (text.empty()) {return true;}
    else if (text == "0") {out = false; return true; }
    else if ( text == "-1" ) {out = true; return true;}

    return false; 

}



} 
std::optional<SbsMessage> parse_sbs(std::string_view line)
{
    SbsMessage msg;

    std::vector<std::string_view> fields;
    std::size_t start {};

    while (true) {
        std::size_t pos = line.find(',', start);

        if (pos == std::string_view::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        std::string_view field = line.substr(start, pos - start);
        fields.push_back(field);

        start = pos + 1;
    }

    if (fields.size() != kFieldCount) {
        return std::nullopt;
    }

    if (fields[kMessageType] != "MSG") {
        return std::nullopt;
    }

    auto icao = parse_hex(fields[kIcao]);
    if (!icao) {
        return std::nullopt;
    }
    msg.icao = *icao;

    auto trans = parse_trans(fields[ktransmission]);
    if (!trans) {
        return std::nullopt;
    }
    msg.transmission = static_cast<Transmission>(*trans);

    if (!parse_T(fields[kAltitude], msg.altitude_ft) ||
        !parse_T(fields[kGroundSpeed], msg.ground_speed_kt) ||
        !parse_T(fields[kTrack], msg.track_deg) ||
        !parse_T(fields[kLat], msg.lat_deg) ||
        !parse_T(fields[kLon], msg.lon_deg) ||
        !parse_T(fields[kVerticalRate], msg.vertical_rate_fpm) ||
        !parse_text(fields[kCallsign], msg.callsign) ||
        !parse_text(fields[kSquawk], msg.squawk) ||
        !parse_flags(fields[kAlert], msg.alert) ||
        !parse_flags(fields[kEmergency], msg.emergency) ||
        !parse_flags(fields[kSpi], msg.spi) ||
        !parse_flags(fields[kOnGround], msg.on_ground)) {
        return std::nullopt;
    }

    return msg;
}
