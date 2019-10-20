package prometheus

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"strings"

	prom "github.com/m3db/prometheus_client_golang/prometheus"
)

// Configuration is a configuration for a Prometheus reporter.
type Configuration struct {
	// HandlerPath if specified will be used instead of using the default
	// HTTP handler path "/metrics".
	HandlerPath string `yaml:"handlerPath"`

	// ListenAddress if specified will be used instead of just registering the
	// handler on the default HTTP serve mux without listening.
	ListenAddress string `yaml:"listenAddress"`

	// TimerType is the default Prometheus type to use for metric timers.
	TimerType string `yaml:"timerType"`

	// DefaultHistogramBuckets if specified will set the default histogram
	// buckets to be used by the reporter.
	DefaultHistogramBuckets []HistogramObjective `yaml:"defaultHistogramBuckets"`

	// DefaultSummaryObjectives if specified will set the default summary
	// objectives to be used by the reporter.
	DefaultSummaryObjectives []SummaryObjective `yaml:"defaultSummaryObjectives"`

	// OnError specifies what to do when an error either with listening
	// on the specified listen address or registering a metric with the
	// Prometheus. By default the registerer will panic.
	OnError string `yaml:"onError"`
}

// HistogramObjective is a Prometheus histogram bucket.
// See: https://godoc.org/github.com/prometheus/client_golang/prometheus#HistogramOpts
type HistogramObjective struct {
	Upper float64 `yaml:"upper"`
}

// SummaryObjective is a Prometheus summary objective.
// See: https://godoc.org/github.com/prometheus/client_golang/prometheus#SummaryOpts
type SummaryObjective struct {
	Percentile   float64 `yaml:"percentile"`
	AllowedError float64 `yaml:"allowedError"`
}

// ConfigurationOptions allows some programatic options, such as using a
// specific registry and what error callback to register.
type ConfigurationOptions struct {
	// Registry if not nil will specify the specific registry to use
	// for registering metrics.
	Registry *prom.Registry
	// OnError allows for customization of what to do when a metric
	// registration error fails, the default is to panic.
	OnError func(e error)
}

// NewReporter creates a new M3 reporter from this configuration.
func (c Configuration) NewReporter(
	configOpts ConfigurationOptions,
) (Reporter, error) {
	var opts Options

	if configOpts.Registry != nil {
		opts.Registerer = configOpts.Registry
	}

	if configOpts.OnError != nil {
		opts.OnRegisterError = configOpts.OnError
	} else {
		switch c.OnError {
		case "stderr":
			opts.OnRegisterError = func(err error) {
				fmt.Fprintf(os.Stderr, "metrics prometheus reporter error: %v\n", err)
			}
		case "log":
			opts.OnRegisterError = func(err error) {
				log.Printf("metrics prometheus reporter error: %v\n", err)
			}
		case "none":
			opts.OnRegisterError = func(err error) {}
		default:
			opts.OnRegisterError = func(err error) {
				panic(err)
			}
		}
	}

	switch c.TimerType {
	case "summary":
		opts.DefaultTimerType = SummaryTimerType
	case "histogram":
		opts.DefaultTimerType = HistogramTimerType
	}

	if len(c.DefaultHistogramBuckets) > 0 {
		var values []float64
		for _, value := range c.DefaultHistogramBuckets {
			values = append(values, value.Upper)
		}
		opts.DefaultHistogramBuckets = values
	}

	if len(c.DefaultSummaryObjectives) > 0 {
		values := make(map[float64]float64)
		for _, value := range c.DefaultSummaryObjectives {
			values[value.Percentile] = value.AllowedError
		}
		opts.DefaultSummaryObjectives = values
	}

	reporter := NewReporter(opts)

	path := "/metrics"
	if handlerPath := strings.TrimSpace(c.HandlerPath); handlerPath != "" {
		path = handlerPath
	}

	if addr := strings.TrimSpace(c.ListenAddress); addr == "" {
		http.Handle(path, reporter.HTTPHandler())
	} else {
		mux := http.NewServeMux()
		mux.Handle(path, reporter.HTTPHandler())
		go func() {
			if err := http.ListenAndServe(addr, mux); err != nil {
				opts.OnRegisterError(err)
			}
		}()
	}

	return reporter, nil
}
