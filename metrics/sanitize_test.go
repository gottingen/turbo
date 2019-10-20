package metrics

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func newTestSanitizer() SanitizeFn {
	c := &ValidCharacters{
		Ranges:     AlphanumericRange,
		Characters: UnderscoreDashCharacters,
	}
	return c.sanitizeFn(DefaultReplacementCharacter)
}

func TestSanitizeIdentifierAllValidCharacters(t *testing.T) {
	allValidChars := "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
	fn := newTestSanitizer()
	require.Equal(t, allValidChars, fn(allValidChars))
}

func TestSanitizeTestCases(t *testing.T) {
	fn := newTestSanitizer()
	type testCase struct {
		input  string
		output string
	}

	testCases := []testCase{
		{"abcdef0AxS-s_Z", "abcdef0AxS-s_Z"},
		{"a:b", "a_b"},
		{"a! b", "a__b"},
		{"?bZ", "_bZ"},
	}

	for _, tc := range testCases {
		require.Equal(t, tc.output, fn(tc.input))
	}
}
