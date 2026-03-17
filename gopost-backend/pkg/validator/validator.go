package validator

import (
	"regexp"

	"github.com/gopost-app/gopost-backend/pkg/response"
)

var emailRegex = regexp.MustCompile(`^[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}$`)

func ValidateEmail(email string) *response.FieldError {
	if email == "" {
		return &response.FieldError{Field: "email", Message: "Email is required"}
	}
	if !emailRegex.MatchString(email) {
		return &response.FieldError{Field: "email", Message: "Invalid email format"}
	}
	return nil
}

func ValidatePassword(password string) *response.FieldError {
	if password == "" {
		return &response.FieldError{Field: "password", Message: "Password is required"}
	}
	if len(password) < 8 {
		return &response.FieldError{Field: "password", Message: "Password must be at least 8 characters"}
	}
	return nil
}

func ValidateRequired(field, value string) *response.FieldError {
	if value == "" {
		return &response.FieldError{Field: field, Message: field + " is required"}
	}
	return nil
}
