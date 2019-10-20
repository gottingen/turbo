package main

import (
	"fmt"
	"math/rand"
	"net/http"
	"time"

	"github.com/gottingen/kmetrics/metrics"
	promreporter "github.com/gottingen/kmetrics/metrics/prometheus"
)

func main() {
	r := promreporter.NewReporter(promreporter.Options{})

	// Note: `promreporter.DefaultSeparator` is "_".
	// Prometheus doesnt like metrics with "." or "-" in them.
	scope, closer := metrics.NewRootScope(metrics.ScopeOptions{
		Prefix:         "my_service",
		Tags:           map[string]string{},
		CachedReporter: r,
		Separator:      promreporter.DefaultSeparator,
	}, 1*time.Second)
	defer closer.Close()

	counter := scope.Tagged(map[string]string{
		"foo": "bar",
	}).Counter("test_counter")

	gauge := scope.Tagged(map[string]string{
		"foo": "baz",
	}).Gauge("test_gauge")

	timer := scope.Tagged(map[string]string{
		"foo": "qux",
	}).Timer("test_timer_summary")

	histogram := scope.Tagged(map[string]string{
		"foo": "quk",
	}).Histogram("test_histogram", metrics.DefaultBuckets)

	igauge := scope.Tagged(map[string]string{
		"foo": "qug",
	}).IntegerGauge("test_integer_gauge")

	go func() {
		for {
			counter.Inc(1)
			igauge.Inc(2)
			time.Sleep(time.Second)
		}
	}()

	go func() {
		for {
			gauge.Update(rand.Float64() * 1000)
			time.Sleep(time.Second)
		}
	}()

	go func() {
		for {
			tsw := timer.Start()
			hsw := histogram.Start()
			time.Sleep(time.Duration(rand.Float64() * float64(time.Second)))
			tsw.Stop()
			hsw.Stop()
		}
	}()

	http.Handle("/metrics", r.HTTPHandler())
	fmt.Printf("Serving :8080/metrics\n")
	fmt.Printf("%v\n", http.ListenAndServe(":8080", nil))
	select {}
}
