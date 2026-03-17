package entity

import (
	"time"

	"github.com/google/uuid"
)

type User struct {
	ID                uuid.UUID  `json:"id"`
	Email             string     `json:"email"`
	PasswordHash      string     `json:"-"`
	Name              string     `json:"name"`
	AvatarURL         string     `json:"avatar_url,omitempty"`
	OAuthProvider     string     `json:"oauth_provider,omitempty"`
	OAuthID           string     `json:"oauth_id,omitempty"`
	DeviceFingerprint string     `json:"-"`
	CreatedAt         time.Time  `json:"created_at"`
	UpdatedAt         time.Time  `json:"updated_at"`
	DeletedAt         *time.Time `json:"deleted_at,omitempty"`
}
