package entity

import (
	"time"

	"github.com/google/uuid"
)

type Session struct {
	ID                uuid.UUID `json:"id"`
	UserID            uuid.UUID `json:"user_id"`
	RefreshTokenHash  string    `json:"-"`
	DeviceFingerprint string    `json:"device_fingerprint,omitempty"`
	IPAddress         string    `json:"ip_address,omitempty"`
	UserAgent         string    `json:"user_agent,omitempty"`
	ExpiresAt         time.Time `json:"expires_at"`
	CreatedAt         time.Time `json:"created_at"`
}
