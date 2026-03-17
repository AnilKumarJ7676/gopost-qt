package entity

import "github.com/google/uuid"

type Category struct {
	ID          uuid.UUID `json:"id"`
	Name        string    `json:"name"`
	Slug        string    `json:"slug"`
	Description string    `json:"description,omitempty"`
	IconURL     string    `json:"icon_url,omitempty"`
	SortOrder   int       `json:"sort_order"`
	IsActive    bool      `json:"is_active"`
}
