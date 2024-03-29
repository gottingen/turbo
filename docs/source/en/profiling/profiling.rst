.. Copyright 2023 The Elastic AI Search Authors.


write most variables
==========================================
When the application is running, there are usually some statistics that we need to
record when the program is running, such as QPS and Latency. These variables are usually
written to frequently but rarely read. For example, QPS, which can reach more than 10,000
QPS through one application, is a relatively high application. This means that every second,
QPS information, delays, etc., monitoring statistics or other statistics will be recorded 10,000
times, and it will take at least 5 seconds to do it.

In this scenario, the frequency of writing is very high, and data competition protection is essential.
But statistics like these are important. This can usually be achieved using ``std::mutex`` or
``std::atomic``. But this will also introduce new problems. The consumption of statistics in the
competition state will greatly affect the performance of the program. But in fact, these statistics
are indispensable for applied statistics, but they have nothing to do with business.

Carefully analyze the statistical requirements. In fact, the statistical requirements do not need to be
completely accurate (but it cannot be executed arbitrarily in a multi-threaded environment without
protection, and the data is completely untrustworthy). One conclusion is drawn that real-time
statistics require "approximately accurate data". Taking QPS as an example, in fact, the QPS displayed
by the monitoring system is also approximately "seconds". Based on this, an approximate statistical
scheme is designed.

For statistical variables, each thread copies a copy of the data. When writing, only the local data
of this thread is written. When reading, the variables of multiple threads are aggregated. For example,
QPS uses cumulative aggregation, and latency uses average aggregation.

General, statistics can be operated atomically. Reading and writing are guaranteed through atomic
reading and writing.

The operation time of atomic operations is at the nanosecond level. The number of application threads
is usually dozens, let’s count it as 100. Taking a variable as an example, the time to read a variable
is at most 1 microsecond. Compared with using a global lock to lock the read When taking data, the data
error is one in a million branches. Usually, the error in business statistics reaches one thousandth,
which is tolerable.

What will happen if the simplest global lock or atomic variable is used? First of all, if the
statistic is a global variable protected by a mutex lock, in a multi-threaded scenario, experiments
have been done and the update frequency of the variable will decrease as the number of threads
increases. When the number of threads reaches more than 20, the global The update frequency can
only be in the millions. This will seriously affect application performance. According to
monitoring, the CPU utilization will be reduced and the application performance cannot be provided.


If you are interested in this, you can learn about
`TMAN <https://www.intel.com/content/www/us/en/docs/vtune-profiler/cookbook/2023-0/top-down-microarchitecture-analysis-method.html>`_.
There is also an open source tool `pm-tools <https://github.com/andikleen/pmu-tools>`_. This tool can help
you analyze the performance of the application. You can use this tool to analyze the performance of the application
and find the performance bottleneck.  ``pm-tools`` do not have deb package or pip package, download it form
github and set to $PATH.

Via the pm-tools you will see, the cores are blocked by the atomic operations or mutex lock. The application
is not CPU bounded, but memory bounded. The application is blocked by the memory access. The memory access
is blocked by the atomic operations or mutex lock. The application is not CPU bounded, but memory bounded.


about metrics concept
=============================================

to be uniform, we use the following concepts:

metric: a variable that is used to record the state of the application. For example, QPS, latency, etc.

* Counter metric: a metric that is used to record the number of occurrences of an event. For example, the number of
  requests, the number of errors, etc.

* Gauge metric: a metric that is used to record the current state of the application. For example, the number of
  threads, the number of connections, etc.

* Histogram metric: a metric that is used to record the distribution of a variable. For example, the distribution of
    latency, the distribution of the number of requests, etc.

using metrics
================================================

the metrics is very easy to use. you can define a metric in global scope, and write it int the application
anywhere. the metrics is thread safe. you can write it in multi-threaded environment.

.. note::

    the metrics is write light, read heavy. do not read it frequently as a getter. the metrics is not
    designed for this. the metrics is designed for write frequently, read occasionally.

let's see how to use the metrics.

.. code-block:: c++

    #include <turbo/profiling/variable.h>

    // define a counter metric
    turbo::Counter<std::int64_t> g_counter("counter", "counter description");
    turbo::MaxerGauge<std::int64_t> g_maxer_gauge("maxer_gauge", "maxer gauge description");
    turbo::MinerGauge<std::int64_t> g_miner_gauge("miner_gauge", "miner gauge description");

    // define a histogram metric
    turbo::Histogram<std::int64_t> g_histogram("histogram", "histogram description", {0, 10, 20, 30, 40, 50, 60, 70, 80, 90});

    // write the metrics
    g_counter.inc();
    g_maxer_gauge.set(100);
    g_miner_gauge.set(100);
    g_histogram.observe(100);

    // read the metrics
    std::cout << g_counter.get() << std::endl;
    std::cout << g_maxer_gauge.get() << std::endl;
    std::cout << g_miner_gauge.get() << std::endl;
    std::cout << g_histogram.get() << std::endl;

    // dump the metrics
    std::cout << g_counter.dump() << std::endl;
    std::cout << g_maxer_gauge.dump() << std::endl;
    std::cout << g_miner_gauge.dump() << std::endl;
    std::cout << g_histogram.dump() << std::endl;

    // dump the metrics to json
    std::cout << g_counter.dump_json() << std::endl;
    std::cout << g_maxer_gauge.dump_json() << std::endl;
    std::cout << g_miner_gauge.dump_json() << std::endl;
    std::cout << g_histogram.dump_json() << std::endl;

    // dump the metrics to prometheus
    turbo::PrometheusDump prometheus_dump;
    std::cout << g_counter.dump_prometheus(prometheus_dump) << std::endl;
    std::cout << g_maxer_gauge.dump_prometheus(prometheus_dump) << std::endl;
    std::cout << g_miner_gauge.dump_prometheus(prometheus_dump) << std::endl;
    std::cout << g_histogram.dump

see a example usually in actual application

.. code-block:: c++

    #include <turbo/profiling/variable.h>
    #include <turbo/profiling/prometheus_dump.h>
    #include <turbo/profiling/counter.h>
    #include <turbo/profiling/maxer_gauge.h>
    #include <turbo/profiling/miner_gauge.h>
    #include <turbo/profiling/histogram.h>
    #include <turbo/random/random.h>

    // define a counter metric
    turbo::Counter<std::int64_t> g_counter("counter", "counter description");
    turbo::MaxerGauge<std::int64_t> g_maxer_gauge("maxer_gauge", "maxer gauge description");
    turbo::MinerGauge<std::int64_t> g_miner_gauge("miner_gauge", "miner gauge description");

    // define a histogram metric
    turbo::Histogram<std::int64_t> g_histogram("histogram", "histogram description", {0, 10, 20, 30, 40, 50, 60, 70, 80, 90});

    // define a thread to write the metrics
    bool g_running = true;
    void write_metrics() {
        while (g_running) {
            g_counter.inc();
            g_maxer_gauge.set(turbo::uniform<uint32_t>(50, 100));
            g_miner_gauge.set(turbo::uniform<uint32_t>(0, 50));
            auto s = g_histogram.scope_latency_double_milliseconds();
            // do something
        }
    };
    auto t0 = std::thread(write_metrics);
    auto t1 = std::thread(write_metrics);
    auto t2 = std::thread(write_metrics);
    auto t3 = std::thread(write_metrics);
    while (is_ask_to_stop() == false) {
        // std::cout << g_counter.dump() << std::endl;
        // std::cout << g_maxer_gauge.dump() << std::endl;
        // std::cout << g_miner_gauge.dump() << std::endl;
        // std::cout << g_histogram.dump() << std::endl;
        auto str = Variable::dump_prometheus_all();
        // report str to prometheus server
        // or dump it when prometheus server request and response it
        turbo::sleep_for(turbo::Duration::seconds(10));
    }
    g_running = false;
    t0.join();
    t1.join();
    t2.join();
    t3.join();




prometheus
=========================================

Prometheus is a monitoring system and time series database. It is a very popular monitoring system. It is also a
very good time series database. It is very suitable for storing time series data. It is also very suitable for
querying time series data. It is also very suitable for visualizing time series data. It is also very suitable
for alerting time series data. It is also very suitable for recording time series data. It is also very suitable

turbo has supported prometheus. the ``Variable`` have supported  to dump the data to prometheus. details to see
``Variable`` api section.

dump the data to prometheus

.. code-block:: c++

    #include <turbo/profiling/prometheus_dump.h>
    #include <turbo/profiling/variable.h>

    // define a counter metric
    turbo::Counter<std::int64_t> g_counter("counter", "counter description");
    turbo::MaxerGauge<std::int64_t> g_maxer_gauge("maxer_gauge", "maxer gauge description");
    turbo::MinerGauge<std::int64_t> g_miner_gauge("miner_gauge", "miner gauge description");

    // define a histogram metric
    turbo::Histogram<std::int64_t> g_histogram("histogram", "histogram description", {0, 10, 20, 30, 40, 50, 60, 70, 80, 90});

    // dump the data to prometheus
    turbo::PrometheusDump prometheus_dump;
    auto str = Variable::dump_prometheus_all(prometheus_dump);
    // do something with str




