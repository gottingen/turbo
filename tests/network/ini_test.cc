
#include <iostream>
#include "turbo/network/util/logger.h"
#include "turbo/network/util/mini.h"
using namespace std;
using namespace turbo;

int main() {
    //初始化日志系统
    Logger::Instance().add(std::make_shared<ConsoleChannel> ());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());
    mINI ini;
    ini[".dot"] = "dot-value";
    ini["no-dot"] = "no-dot-value";
    ini["no-key-filed."] = "no-key-value";
    ini["field0.multi.dot"] = "filed.multi.dot-value";

    ini["field0.str"] = "value";
    ini["field0.int"] = 1;
    ini["field0.bool"] = true;

    ini["field1.str"] = "value";
    ini["field1.int"] = 1;
    ini["field1.bool"] = true;

    auto str = ini.dump();
    InfoL << "\n" << str;

    ini.clear();
    ini.parse(str);
    for (auto &pr: ini) {
        DebugL << pr.first << " = " << pr.second;
    }

    auto ini_str = R"(
        no—field=value

        [filed]
        a-key
        b-key=
        c-key=test
        ; comment0
        d-key = test
        # comment1
        e-key =
        =no-key
        multi.dot=multi.dot.value
    )";
    ini.clear();
    ini.parse(ini_str);
    for (auto &pr: ini) {
        TraceL << pr.first << " = " << pr.second;
    }

    return 0;
}
