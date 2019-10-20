package metrics

import (
	"bytes"
)

var (
	// DefaultReplacementCharacter is the default character used for
	// replacements.
	DefaultReplacementCharacter = '_'

	// AlphanumericRange is the range of alphanumeric characters.
	AlphanumericRange = []SanitizeRange{
		{rune('a'), rune('z')},
		{rune('A'), rune('Z')},
		{rune('0'), rune('9')}}

	// UnderscoreCharacters is just an underscore character.
	UnderscoreCharacters = []rune{
		'_'}

	// UnderscoreDashCharacters is a slice of underscore, and
	// dash characters.
	UnderscoreDashCharacters = []rune{
		'-',
		'_'}

	// UnderscoreDashDotCharacters is a slice of underscore,
	// dash, and dot characters.
	UnderscoreDashDotCharacters = []rune{
		'.',
		'-',
		'_'}
)

// SanitizeFn returns a sanitized version of the input string.
type SanitizeFn func(string) string

// SanitizeRange is a range of characters (inclusive on both ends).
type SanitizeRange [2]rune

// ValidCharacters is a collection of valid characters.
type ValidCharacters struct {
	Ranges     []SanitizeRange
	Characters []rune
}

// SanitizeOptions are the set of configurable options for sanitisation.
type SanitizeOptions struct {
	NameCharacters       ValidCharacters
	KeyCharacters        ValidCharacters
	ValueCharacters      ValidCharacters
	ReplacementCharacter rune
}

// Sanitizer sanitizes the provided input based on the function executed.
type Sanitizer interface {
	// Name sanitizes the provided `name` string.
	Name(n string) string

	// Key sanitizes the provided `key` string.
	Key(k string) string

	// Value sanitizes the provided `value` string.
	Value(v string) string
}

// NewSanitizer returns a new sanitizer based on provided options.
func NewSanitizer(opts SanitizeOptions) Sanitizer {
	return sanitizer{
		nameFn:  opts.NameCharacters.sanitizeFn(opts.ReplacementCharacter),
		keyFn:   opts.KeyCharacters.sanitizeFn(opts.ReplacementCharacter),
		valueFn: opts.ValueCharacters.sanitizeFn(opts.ReplacementCharacter),
	}
}

// NoOpSanitizeFn returns the input un-touched.
func NoOpSanitizeFn(v string) string { return v }

// NewNoOpSanitizer returns a sanitizer which returns all inputs un-touched.
func NewNoOpSanitizer() Sanitizer {
	return sanitizer{
		nameFn:  NoOpSanitizeFn,
		keyFn:   NoOpSanitizeFn,
		valueFn: NoOpSanitizeFn,
	}
}

type sanitizer struct {
	nameFn  SanitizeFn
	keyFn   SanitizeFn
	valueFn SanitizeFn
}

func (s sanitizer) Name(n string) string {
	return s.nameFn(n)
}

func (s sanitizer) Key(k string) string {
	return s.keyFn(k)
}

func (s sanitizer) Value(v string) string {
	return s.valueFn(v)
}

func (c *ValidCharacters) sanitizeFn(repChar rune) SanitizeFn {
	return func(value string) string {
		var buf *bytes.Buffer
		for idx, ch := range value {
			// first check if the provided character is valid
			validCurr := false
			for i := 0; !validCurr && i < len(c.Ranges); i++ {
				if ch >= c.Ranges[i][0] && ch <= c.Ranges[i][1] {
					validCurr = true
					break
				}
			}
			for i := 0; !validCurr && i < len(c.Characters); i++ {
				if c.Characters[i] == ch {
					validCurr = true
					break
				}
			}

			// if it's valid, we can optimise allocations by avoiding copying
			if validCurr {
				if buf == nil {
					continue // haven't deviated from string, still no need to init buffer
				}
				buf.WriteRune(ch) // we've deviated from string, write to buffer
				continue
			}

			// ie the character is invalid, and the buffer has not been initialised
			// so we initialise buffer and backfill
			if buf == nil {
				buf = bytes.NewBuffer(make([]byte, 0, len(value)))
				if idx > 0 {
					buf.WriteString(value[:idx])
				}
			}

			// write the replacement character
			buf.WriteRune(repChar)
		}

		// return input un-touched if the buffer has been not initialised
		if buf == nil {
			return value
		}

		// otherwise, return the newly constructed buffer
		return buf.String()
	}
}
