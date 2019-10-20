package prometheus

import (
	"github.com/gottingen/kmetrics/metrics"
)

var (
	// DefaultSanitizerOpts are the options for the default Prometheus sanitizer.
	DefaultSanitizerOpts = metrics.SanitizeOptions{
		NameCharacters: metrics.ValidCharacters{
			Ranges:     metrics.AlphanumericRange,
			Characters: metrics.UnderscoreCharacters,
		},
		KeyCharacters: metrics.ValidCharacters{
			Ranges:     metrics.AlphanumericRange,
			Characters: metrics.UnderscoreCharacters,
		},
		ValueCharacters: metrics.ValidCharacters{
			Ranges:     metrics.AlphanumericRange,
			Characters: metrics.UnderscoreCharacters,
		},
		ReplacementCharacter: metrics.DefaultReplacementCharacter,
	}
)
