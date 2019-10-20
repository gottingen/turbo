package metrics

import "time"

// BaseStatsReporter implements the shared reporter methods.
type BaseStatsReporter interface {
	// Capabilities returns the capabilities description of the reporter.
	Capabilities() Capabilities

	// Flush asks the reporter to flush all reported values.
	Flush()
}

// StatsReporter is a backend for Scopes to report metrics to.
type StatsReporter interface {
	BaseStatsReporter

	// ReportCounter reports a counter value
	ReportCounter(
		name string,
		tags map[string]string,
		value int64,
	)

	// ReportGauge reports a gauge value
	ReportGauge(
		name string,
		tags map[string]string,
		value float64,
	)

	// ReportTimer reports a timer value
	ReportTimer(
		name string,
		tags map[string]string,
		interval time.Duration,
	)

	// ReportHistogramValueSamples reports histogram samples for a bucket
	ReportHistogramValueSamples(
		name string,
		tags map[string]string,
		buckets Buckets,
		bucketLowerBound,
		bucketUpperBound float64,
		samples int64,
	)

	// ReportHistogramDurationSamples reports histogram samples for a bucket
	ReportHistogramDurationSamples(
		name string,
		tags map[string]string,
		buckets Buckets,
		bucketLowerBound,
		bucketUpperBound time.Duration,
		samples int64,
	)
}

// CachedStatsReporter is a backend for Scopes that pre allocates all
// counter, gauges, timers & histograms. This is harder to implement but more performant.
type CachedStatsReporter interface {
	BaseStatsReporter

	// AllocateCounter pre allocates a counter data structure with name & tags.
	AllocateCounter(
		name string,
		tags map[string]string,
	) CachedCount

	// AllocateGauge pre allocates a gauge data structure with name & tags.
	AllocateGauge(
		name string,
		tags map[string]string,
	) CachedGauge

	// AllocateTimer pre allocates a timer data structure with name & tags.
	AllocateTimer(
		name string,
		tags map[string]string,
	) CachedTimer

	// AllocateHistogram pre allocates a histogram data structure with name, tags,
	// value buckets and duration buckets.
	AllocateHistogram(
		name string,
		tags map[string]string,
		buckets Buckets,
	) CachedHistogram
}

// CachedCount interface for reporting an individual counter
type CachedCount interface {
	ReportCount(value int64)
}

// CachedGauge interface for reporting an individual gauge
type CachedGauge interface {
	ReportGauge(value float64)
}

// CachedTimer interface for reporting an individual timer
type CachedTimer interface {
	ReportTimer(interval time.Duration)
}

// CachedHistogram interface for reporting histogram samples to buckets
type CachedHistogram interface {
	ValueBucket(
		bucketLowerBound, bucketUpperBound float64,
	) CachedHistogramBucket
	DurationBucket(
		bucketLowerBound, bucketUpperBound time.Duration,
	) CachedHistogramBucket
}

// CachedHistogramBucket interface for reporting histogram samples to a specific bucket
type CachedHistogramBucket interface {
	ReportSamples(value int64)
}
