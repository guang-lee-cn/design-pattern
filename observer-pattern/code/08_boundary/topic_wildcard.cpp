// The list itself can change shape: from a list of names to a list of rules.
//
// Form C moved the receiver list out of the producer. This moves something else: the
// entries. A subscription is no longer "the topic temperature/room1" but "anything one
// level under temperature". The publisher still names exactly one topic -- and now it
// cannot even enumerate who will hear it, because the answer depends on a match it does
// not perform.
//
// Build: g++ -std=c++17 -Wall -Wextra topic_wildcard.cpp -o /tmp/fw && /tmp/fw

#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class TopicBus {
 public:
  using Handler = std::function<void(double)>;

  void Subscribe(std::string pattern, Handler handler) {
    subscriptions_.push_back(Subscription{std::move(pattern), std::move(handler)});
  }

  // The publisher names one topic. It does not name, and cannot count, its audience.
  void Publish(const std::string& topic, double value) {
    std::printf("-- publish %s %.1f\n", topic.c_str(), value);
    int matched = 0;
    for (const Subscription& sub : subscriptions_) {
      if (Matches(sub.pattern, topic)) {
        sub.handler(value);
        ++matched;
      }
    }
    std::printf("   %d of %zu subscriptions matched, chosen by rule\n", matched,
                subscriptions_.size());
  }

 private:
  struct Subscription {
    std::string pattern;
    Handler handler;
  };

  static std::vector<std::string> Split(const std::string& text) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : text) {
      if (c == '/') {
        parts.push_back(current);
        current.clear();
      } else {
        current += c;
      }
    }
    parts.push_back(current);
    return parts;
  }

  // '*' matches exactly one level, '**' matches everything from here on.
  static bool Matches(const std::string& pattern, const std::string& topic) {
    const std::vector<std::string> p = Split(pattern);
    const std::vector<std::string> t = Split(topic);
    for (std::size_t i = 0; i < p.size(); ++i) {
      if (p[i] == "**") return true;
      if (i >= t.size()) return false;
      if (p[i] != "*" && p[i] != t[i]) return false;
    }
    return p.size() == t.size();
  }

  std::vector<Subscription> subscriptions_;
};

int main() {
  TopicBus bus;

  bus.Subscribe("temperature/room1", [](double value) {
    std::printf("   [exact     ] temperature/room1 -> %.1f\n", value);
  });
  bus.Subscribe("temperature/*", [](double value) {
    std::printf("   [one level ] temperature/*     -> %.1f\n", value);
  });
  bus.Subscribe("**", [](double value) {
    std::printf("   [everything] **                -> %.1f\n", value);
  });

  bus.Publish("temperature/room1", 25.0);
  bus.Publish("temperature/room2", 26.5);
  bus.Publish("humidity/room1", 41.0);
  return 0;
}
