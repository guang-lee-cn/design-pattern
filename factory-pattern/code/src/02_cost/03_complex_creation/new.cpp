// src/02_cost/03_complex_creation/new.cpp
//
// Same scenario with a factory that owns the connection lifecycle. Callers
// request a connection by logical database name; the factory creates it once
// and returns the same instance afterwards. The expensive open happens once
// per logical database, and reuse / cap policy now lives in one place.
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "constants.h"

namespace {

using factory_pattern::kDbAnalytics;
using factory_pattern::kDbPrimary;

class DbConnection {
public:
    explicit DbConnection(std::string_view db) : db_(db) {
        ++opened_count_;
        std::printf("[resource] opened connection to %s\n", db_.c_str());
    }

    void Execute(std::string_view statement) const {
        (void)statement;
    }

    static int opened_count() {
        return opened_count_;
    }

private:
    std::string db_;
    static inline int opened_count_ = 0;
};

// Owns every connection and guarantees one per logical database name.
class ConnectionFactory {
public:
    static DbConnection &Get(std::string_view db) {
        std::string key(db);
        auto found = cache_.find(key);
        if (found != cache_.end()) {
            std::printf("[resource] reused connection to %s\n", key.c_str());
            return *found->second;
        }
        auto connection   = std::make_unique<DbConnection>(db);
        DbConnection *raw = connection.get();
        cache_.emplace(key, std::move(connection));
        return *raw;
    }

private:
    static inline std::unordered_map<std::string, std::unique_ptr<DbConnection>> cache_;
};

// Operations ask the factory; they no longer decide how to build connections.
void RecordOrder() {
    DbConnection &connection = ConnectionFactory::Get(kDbPrimary);
    connection.Execute("INSERT INTO orders ...");
    std::printf("order recorded\n");
}

void AppendAuditLog() {
    DbConnection &connection = ConnectionFactory::Get(kDbPrimary);
    connection.Execute("INSERT INTO audit ...");
    std::printf("audit log appended\n");
}

void FetchBalance() {
    DbConnection &connection = ConnectionFactory::Get(kDbPrimary);
    connection.Execute("SELECT balance ...");
    std::printf("balance fetched\n");
}

void GenerateReport() {
    DbConnection &connection = ConnectionFactory::Get(kDbAnalytics);
    connection.Execute("SELECT sum(...) ...");
    std::printf("report generated\n");
}

} // namespace

int main() {
    RecordOrder();
    AppendAuditLog();
    FetchBalance();
    GenerateReport();
    std::printf("[resource] total connections opened: %d\n", DbConnection::opened_count());
}
