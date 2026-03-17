package middleware

import (
	"context"
	"fmt"
	"net/http"
	"strconv"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/cache"
	"github.com/gopost-app/gopost-backend/pkg/response"
)

type RateLimitConfig struct {
	Limit  int
	Window time.Duration
}

var RateLimitPresets = map[string]RateLimitConfig{
	"auth":     {Limit: 10, Window: time.Minute},
	"browsing": {Limit: 120, Window: time.Minute},
	"upload":   {Limit: 20, Window: time.Minute},
	"default":  {Limit: 60, Window: time.Minute},
}

func RateLimit(rdb *cache.Redis, category string) gin.HandlerFunc {
	cfg, ok := RateLimitPresets[category]
	if !ok {
		cfg = RateLimitPresets["default"]
	}

	return func(c *gin.Context) {
		identifier := c.ClientIP()
		if uid, exists := c.Get("user_id"); exists {
			identifier = fmt.Sprintf("user:%v", uid)
		}

		key := fmt.Sprintf("ratelimit:%s:%s", category, identifier)
		ctx := context.Background()

		count, err := rdb.Client.Incr(ctx, key).Result()
		if err != nil {
			c.Next()
			return
		}

		if count == 1 {
			rdb.Client.Expire(ctx, key, cfg.Window)
		}

		ttl, _ := rdb.Client.TTL(ctx, key).Result()
		remaining := cfg.Limit - int(count)
		if remaining < 0 {
			remaining = 0
		}

		c.Header("X-RateLimit-Limit", strconv.Itoa(cfg.Limit))
		c.Header("X-RateLimit-Remaining", strconv.Itoa(remaining))
		c.Header("X-RateLimit-Reset", strconv.FormatInt(time.Now().Add(ttl).Unix(), 10))

		if int(count) > cfg.Limit {
			c.Header("Retry-After", strconv.Itoa(int(ttl.Seconds())))
			response.Error(c, http.StatusTooManyRequests, "RATE_LIMITED", "Too many requests, please try again later")
			c.Abort()
			return
		}

		c.Next()
	}
}
