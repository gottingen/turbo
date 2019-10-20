package prometheus

import (
	"net"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestListenErrorCallsOnRegisterError(t *testing.T) {
	// Ensure that Listen error calls default OnRegisterError to panic
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	require.NoError(t, err)

	defer func() { _ = listener.Close() }()

	assert.NotPanics(t, func() {
		cfg := Configuration{
			ListenAddress: listener.Addr().String(),
			OnError:       "log",
		}
		_, _ = cfg.NewReporter(ConfigurationOptions{})
		time.Sleep(time.Second)
	})
}
