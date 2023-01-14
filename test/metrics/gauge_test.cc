
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/sstream_workaround.h"
#include "turbo/metrics/gauge.h"
#include "turbo/metrics/prometheus_dumper.h"
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include "testing/gtest_wrap.h"

TEST(metrics, gauge) {
    turbo::gauge<int64_t> g1("g1","", {{"a","search"}, {"q","qruu"}});
    g1<<1;
    g1<<5;
    EXPECT_EQ(g1.get_value(), 6);
    auto n = turbo::time_now();
    turbo::cache_metrics cm;
    g1.collect_metrics(cm);
    auto t = turbo::time_now();
    auto str = turbo::prometheus_dumper::dump_to_string(cm, &t);
    std::string g1_s = "# HELP g1\n"
                       "# TYPE g1 gauge\n"
                       "g1{a=\"search\",q=\"qruu\"} 6.000000\n";
    std::cout<<g1_s<<std::endl;
    std::cout<<str<<std::endl;
    std::vector<turbo::cache_metrics> vcm;
    turbo::variable_base::list_metrics(&vcm);
    EXPECT_EQ(vcm.size(), 1);
}
