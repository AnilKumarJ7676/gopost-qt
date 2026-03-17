package logger

import (
	"os"
	"time"

	"github.com/rs/zerolog"
)

func New(level string) zerolog.Logger {
	zerolog.TimeFieldFormat = time.RFC3339Nano

	lvl, err := zerolog.ParseLevel(level)
	if err != nil {
		lvl = zerolog.DebugLevel
	}

	return zerolog.New(os.Stdout).
		Level(lvl).
		With().
		Timestamp().
		Caller().
		Logger()
}
