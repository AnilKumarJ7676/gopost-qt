package middleware

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/pkg/response"
	"github.com/rs/zerolog"
)

func Recovery(log zerolog.Logger) gin.HandlerFunc {
	return func(c *gin.Context) {
		defer func() {
			if err := recover(); err != nil {
				log.Error().
					Interface("error", err).
					Str("request_id", c.GetString("request_id")).
					Msg("panic recovered")

				response.Error(c, http.StatusInternalServerError, "INTERNAL_ERROR", "An internal error occurred")
				c.Abort()
			}
		}()
		c.Next()
	}
}
