package metrics

import (
	"fmt"
	"strconv"
	"testing"
)

func BenchmarkNameGeneration(b *testing.B) {
	root, _ := NewRootScope(ScopeOptions{
		Prefix:   "funkytown",
		Reporter: NullStatsReporter,
	}, 0)
	s := root.(*scope)
	for n := 0; n < b.N; n++ {
		s.fullyQualifiedName("take.me.to")
	}
}

func BenchmarkCounterAllocation(b *testing.B) {
	root, _ := NewRootScope(ScopeOptions{
		Prefix:   "funkytown",
		Reporter: NullStatsReporter,
	}, 0)
	s := root.(*scope)

	ids := make([]string, 0, b.N)
	for i := 0; i < b.N; i++ {
		ids = append(ids, fmt.Sprintf("take.me.to.%d", i))
	}
	b.ResetTimer()

	for n := 0; n < b.N; n++ {
		s.Counter(ids[n])
	}
}

func BenchmarkSanitizedCounterAllocation(b *testing.B) {
	root, _ := NewRootScope(ScopeOptions{
		Prefix:          "funkytown",
		Reporter:        NullStatsReporter,
		SanitizeOptions: &alphanumericSanitizerOpts,
	}, 0)
	s := root.(*scope)

	ids := make([]string, 0, b.N)
	for i := 0; i < b.N; i++ {
		ids = append(ids, fmt.Sprintf("take.me.to.%d", i))
	}
	b.ResetTimer()

	for n := 0; n < b.N; n++ {
		s.Counter(ids[n])
	}
}

func BenchmarkNameGenerationTagged(b *testing.B) {
	root, _ := NewRootScope(ScopeOptions{
		Prefix: "funkytown",
		Tags: map[string]string{
			"style":     "funky",
			"hair":      "wavy",
			"jefferson": "starship",
		},
		Reporter: NullStatsReporter,
	}, 0)
	s := root.(*scope)
	for n := 0; n < b.N; n++ {
		s.fullyQualifiedName("take.me.to")
	}
}

func BenchmarkNameGenerationNoPrefix(b *testing.B) {
	root, _ := NewRootScope(ScopeOptions{
		Reporter: NullStatsReporter,
	}, 0)
	s := root.(*scope)
	for n := 0; n < b.N; n++ {
		s.fullyQualifiedName("im.all.alone")
	}
}

func BenchmarkHistogramAllocation(b *testing.B) {
	root, _ := NewRootScope(ScopeOptions{
		Reporter: NullStatsReporter,
	}, 0)
	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		root.Histogram("foo"+strconv.Itoa(i), DefaultBuckets)
	}
}

func BenchmarkHistogramExisting(b *testing.B) {
	root, _ := NewRootScope(ScopeOptions{
		Reporter: NullStatsReporter,
	}, 0)
	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		root.Histogram("foo", DefaultBuckets)
	}
}
