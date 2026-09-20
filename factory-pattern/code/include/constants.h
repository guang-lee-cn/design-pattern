// include/constants.h
//
// Shared constants of the factory-pattern companion project.
#ifndef FACTORY_PATTERN_INCLUDE_CONSTANTS_H_
#define FACTORY_PATTERN_INCLUDE_CONSTANTS_H_

#include <string_view>

namespace factory_pattern {

// Payment channel identifiers used by section 2, scenario 01_select.
// These keys are shared across callers and factories; the human-facing
// display labels stay inside each concrete gateway.
inline constexpr std::string_view kChannelWechat   = "wechat";
inline constexpr std::string_view kChannelAlipay   = "alipay";
inline constexpr std::string_view kChannelUnionPay = "unionpay";

// Logical database identifiers used by section 2, scenario
// 03_complex_creation. Callers ask for a database by role; they never name
// a concrete connection.
inline constexpr std::string_view kDbPrimary   = "primary-db";
inline constexpr std::string_view kDbAnalytics = "analytics-db";

} // namespace factory_pattern

#endif // FACTORY_PATTERN_INCLUDE_CONSTANTS_H_
