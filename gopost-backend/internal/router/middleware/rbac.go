package middleware

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/pkg/response"
)

func RequireRole(roles ...string) gin.HandlerFunc {
	return func(c *gin.Context) {
		userRole, exists := c.Get("user_role")
		if !exists {
			response.Error(c, http.StatusUnauthorized, "UNAUTHORIZED", "Authentication required")
			c.Abort()
			return
		}

		role, ok := userRole.(string)
		if !ok {
			response.Error(c, http.StatusInternalServerError, "INTERNAL_ERROR", "Invalid role type")
			c.Abort()
			return
		}

		for _, allowed := range roles {
			if role == allowed {
				c.Next()
				return
			}
		}

		response.Error(c, http.StatusForbidden, "FORBIDDEN", "Insufficient permissions")
		c.Abort()
	}
}
