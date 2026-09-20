// src/02_cost/03_complex_creation/old.cpp
//
// Scenario: a database connection is expensive to create (network connect
// plus auth handshake), and several operations in one request logically
// share the same database.
//
// Without a factory, every call site constructs its own connection. The
// open cost is paid repeatedly for what is logically one database, and no
// single place can enforce reuse or a connection cap.
#include <cstdio>
#include <string>
#include <string_view>

#include "constants.h"

namespace {

using factory_pattern::kDbAnalytics;
using factory_pattern::kDbPrimary;

// A heavyweight resource: construction stands in for a real network connect
// and authentication handshake.
class DbConnection {
public:
    explicit DbConnection(std::string_view db) : db_(db) {
        ++opened_count_;
        std::printf("[resource] opened connection to %s\n", db_.c_str());
    }

    void Execute(std::string_view statement) const {
        // Business effect only; identical no matter which instance runs it.
        (void)statement;
    }

    static int opened_count() {
        return opened_count_;
    }

private:
    std::string db_;
    static inline int opened_count_ = 0;
};

// Each operation independently creates the connection it needs.
void RecordOrder() {
    DbConnection connection(kDbPrimary);
    connection.Execute("INSERT INTO orders ...");
    std::printf("order recorded\n");
}

void AppendAuditLog() {
    DbConnection connection(kDbPrimary);
    connection.Execute("INSERT INTO audit ...");
    std::printf("audit log appended\n");
}

void FetchBalance() {
    DbConnection connection(kDbPrimary);
    connection.Execute("SELECT balance ...");
    std::printf("balance fetched\n");
}

void GenerateReport() {
    DbConnection connection(kDbAnalytics);
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
