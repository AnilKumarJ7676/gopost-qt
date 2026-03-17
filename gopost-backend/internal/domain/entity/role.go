package entity

import "github.com/google/uuid"

type Role struct {
	ID          uuid.UUID `json:"id"`
	Name        string    `json:"name"`
	Description string    `json:"description,omitempty"`
}

const (
	RoleUser      = "user"
	RoleCreator   = "creator"
	RoleModerator = "moderator"
	RoleAdmin     = "admin"
)
