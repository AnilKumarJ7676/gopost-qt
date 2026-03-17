package response

import (
	"github.com/gin-gonic/gin"
)

type APIResponse struct {
	Success bool        `json:"success"`
	Data    interface{} `json:"data,omitempty"`
	Error   *APIError   `json:"error,omitempty"`
	Meta    *Meta       `json:"meta,omitempty"`
}

type APIError struct {
	Code    string       `json:"code"`
	Message string       `json:"message"`
	Details []FieldError `json:"details,omitempty"`
}

type FieldError struct {
	Field   string `json:"field"`
	Message string `json:"message"`
}

type Meta struct {
	Page       int   `json:"page"`
	PerPage    int   `json:"per_page"`
	Total      int64 `json:"total"`
	TotalPages int   `json:"total_pages"`
}

func OK(c *gin.Context, data interface{}) {
	c.JSON(200, APIResponse{
		Success: true,
		Data:    data,
	})
}

func Created(c *gin.Context, data interface{}) {
	c.JSON(201, APIResponse{
		Success: true,
		Data:    data,
	})
}

func Paginated(c *gin.Context, data interface{}, meta Meta) {
	c.JSON(200, APIResponse{
		Success: true,
		Data:    data,
		Meta:    &meta,
	})
}

func Error(c *gin.Context, status int, code string, message string) {
	c.JSON(status, APIResponse{
		Success: false,
		Error: &APIError{
			Code:    code,
			Message: message,
		},
	})
}

func ValidationError(c *gin.Context, details []FieldError) {
	c.JSON(400, APIResponse{
		Success: false,
		Error: &APIError{
			Code:    "VALIDATION_ERROR",
			Message: "One or more fields are invalid",
			Details: details,
		},
	})
}
