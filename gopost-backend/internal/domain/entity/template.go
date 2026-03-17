package entity

import (
	"time"

	"github.com/google/uuid"
)

type TemplateType string
type TemplateStatus string

const (
	TemplateTypeVideo TemplateType = "video"
	TemplateTypeImage TemplateType = "image"

	TemplateStatusDraft     TemplateStatus = "draft"
	TemplateStatusReview    TemplateStatus = "review"
	TemplateStatusPublished TemplateStatus = "published"
	TemplateStatusArchived  TemplateStatus = "archived"
)

type Template struct {
	ID             uuid.UUID      `json:"id"`
	Name           string         `json:"name"`
	Description    string         `json:"description,omitempty"`
	Type           TemplateType   `json:"type"`
	CategoryID     *uuid.UUID     `json:"category_id,omitempty"`
	CreatorID      *uuid.UUID     `json:"creator_id,omitempty"`
	Status         TemplateStatus `json:"status"`
	StorageKey     string         `json:"storage_key,omitempty"`
	ThumbnailURL   string         `json:"thumbnail_url,omitempty"`
	PreviewURL     string         `json:"preview_url,omitempty"`
	Width          int            `json:"width,omitempty"`
	Height         int            `json:"height,omitempty"`
	DurationMs     int            `json:"duration_ms,omitempty"`
	LayerCount     int            `json:"layer_count"`
	EditableFields interface{}    `json:"editable_fields,omitempty"`
	UsageCount     int            `json:"usage_count"`
	IsPremium      bool           `json:"is_premium"`
	Version        int            `json:"version"`
	CreatedAt      time.Time      `json:"created_at"`
	UpdatedAt      time.Time      `json:"updated_at"`
	PublishedAt    *time.Time     `json:"published_at,omitempty"`
	Tags           []Tag          `json:"tags,omitempty"`
	Category       *Category      `json:"category,omitempty"`
}

type TemplateVersion struct {
	ID            uuid.UUID `json:"id"`
	TemplateID    uuid.UUID `json:"template_id"`
	VersionNumber int       `json:"version_number"`
	StorageKey    string    `json:"storage_key"`
	Changelog     string    `json:"changelog,omitempty"`
	CreatedAt     time.Time `json:"created_at"`
}

type TemplateAsset struct {
	ID          uuid.UUID `json:"id"`
	TemplateID  uuid.UUID `json:"template_id"`
	AssetType   string    `json:"asset_type"`
	StorageKey  string    `json:"storage_key"`
	ContentHash string    `json:"content_hash,omitempty"`
	FileSize    int64     `json:"file_size"`
	CreatedAt   time.Time `json:"created_at"`
}
