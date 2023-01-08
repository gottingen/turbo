
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/metrics/counter.h"
#include "flare/metrics/prometheus_dumper.h"
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include "testing/gtest_wrap.h"

TEST(metrics, counter) {
    flare::counter<int64_t> c1("c1","", {{"a","search"}, {"q","qruu"}});
    c1<<1;
    c1<<5;
    EXPECT_EQ(c1.get_value(), 6);
    flare::cache_metrics cm;
    c1.collect_metrics(cm);
    auto t = flare::time_now();
    auto str = flare::prometheus_dumper::dump_to_string(cm,&t);
    std::string c1_s = "# HELP c1\n"
                       "# TYPE c1 counter\n"
                       "c1{a=\"search\",q=\"qruu\"} 6.000000\n";
    std::cout<<c1_s<<std::endl;
    std::cout<<str<<std::endl;
}