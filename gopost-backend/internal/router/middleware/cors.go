package middleware

import (
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/internal/config"
)

func CORS(cfg *config.Config) gin.HandlerFunc {
	return func(c *gin.Context) {
		origin := resolveOrigin(cfg, c.GetHeader("Origin"))

		c.Header("Access-Control-Allow-Origin", origin)
		c.Header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		c.Header("Access-Control-Allow-Headers", "Authorization, Content-Type, X-Request-ID")
		c.Header("Access-Control-Allow-Credentials", "true")
		c.Header("Access-Control-Max-Age", "43200")

		if c.Request.Method == "OPTIONS" {
			c.AbortWithStatus(204)
			return
		}

		c.Next()
	}
}

// resolveOrigin determines the allowed origin.
// Priority: CORS_ORIGINS env var > environment-based defaults.
func resolveOrigin(cfg *config.Config, requestOrigin string) string {
	if cfg.CORSOrigins != "" {
		allowed := strings.Split(cfg.CORSOrigins, ",")
		for _, o := range allowed {
			if strings.TrimSpace(o) == requestOrigin {
				return requestOrigin
			}
		}
		if len(allowed) == 1 && strings.TrimSpace(allowed[0]) == "*" {
			return "*"
		}
		return ""
	}

	defaults := map[string]string{
		"development": "*",
		"staging":     "https://staging.gopost.app",
		"production":  "https://gopost.app",
	}

	origin := defaults[cfg.Env]
	if origin == "" {
		origin = "*"
	}
	return origin
}
