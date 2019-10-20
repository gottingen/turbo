package metrics

import (
	"bytes"
	"sort"
)

const (
	prefixSplitter  = '+'
	keyPairSplitter = ','
	keyNameSplitter = '='
)

var (
	keyGenPool = newKeyGenerationPool(1024, 1024, 32)
	nilString  = ""
)

type keyGenerationPool struct {
	bufferPool  *ObjectPool
	stringsPool *ObjectPool
}

// KeyForStringMap generates a unique key for a map string set combination.
func KeyForStringMap(
	stringMap map[string]string,
) string {
	return KeyForPrefixedStringMap(nilString, stringMap)
}

// KeyForPrefixedStringMap generates a unique key for a
// a prefix and a map string set combination.
func KeyForPrefixedStringMap(
	prefix string,
	stringMap map[string]string,
) string {
	keys := keyGenPool.stringsPool.Get().([]string)
	for k := range stringMap {
		keys = append(keys, k)
	}

	sort.Strings(keys)

	buf := keyGenPool.bufferPool.Get().(*bytes.Buffer)

	if prefix != nilString {
		buf.WriteString(prefix)
		buf.WriteByte(prefixSplitter)
	}

	sortedKeysLen := len(stringMap)
	for i := 0; i < sortedKeysLen; i++ {
		buf.WriteString(keys[i])
		buf.WriteByte(keyNameSplitter)
		buf.WriteString(stringMap[keys[i]])
		if i != sortedKeysLen-1 {
			buf.WriteByte(keyPairSplitter)
		}
	}

	key := buf.String()
	keyGenPool.release(buf, keys)
	return key
}

func newKeyGenerationPool(size, blen, slen int) *keyGenerationPool {
	b := NewObjectPool(size)
	b.Init(func() interface{} {
		return bytes.NewBuffer(make([]byte, 0, blen))
	})

	s := NewObjectPool(size)
	s.Init(func() interface{} {
		return make([]string, 0, slen)
	})

	return &keyGenerationPool{
		bufferPool:  b,
		stringsPool: s,
	}
}

func (s *keyGenerationPool) release(b *bytes.Buffer, strs []string) {
	b.Reset()
	s.bufferPool.Put(b)

	for i := range strs {
		strs[i] = nilString
	}
	s.stringsPool.Put(strs[:0])
}
