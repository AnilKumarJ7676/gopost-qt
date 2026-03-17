package repository

import (
	"context"
	"io"
	"time"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/google/uuid"
)

type UserRepository interface {
	Create(ctx context.Context, user *entity.User) error
	GetByID(ctx context.Context, id uuid.UUID) (*entity.User, error)
	GetByEmail(ctx context.Context, email string) (*entity.User, error)
	GetByOAuth(ctx context.Context, provider, oauthID string) (*entity.User, error)
	Update(ctx context.Context, user *entity.User) error
}

type SessionRepository interface {
	Create(ctx context.Context, session *entity.Session) error
	GetByRefreshTokenHash(ctx context.Context, hash string) (*entity.Session, error)
	DeleteByID(ctx context.Context, id uuid.UUID) error
	DeleteAllByUserID(ctx context.Context, userID uuid.UUID) error
}

type RoleRepository interface {
	GetByName(ctx context.Context, name string) (*entity.Role, error)
	GetUserRoles(ctx context.Context, userID uuid.UUID) ([]entity.Role, error)
	AssignRole(ctx context.Context, userID, roleID uuid.UUID) error
}

type TemplateFilter struct {
	Type       string
	Status     string
	CategoryID *uuid.UUID
	IsPremium  *bool
	CreatorID  *uuid.UUID
	Query      string
	Sort       string // "popular", "newest", "trending"
}

type CursorPagination struct {
	Cursor string
	Limit  int
}

type PaginatedResult[T any] struct {
	Items      []T    `json:"items"`
	NextCursor string `json:"next_cursor,omitempty"`
	HasMore    bool   `json:"has_more"`
	Total      int64  `json:"total"`
}

type TemplateRepository interface {
	Create(ctx context.Context, template *entity.Template) error
	GetByID(ctx context.Context, id uuid.UUID) (*entity.Template, error)
	Update(ctx context.Context, template *entity.Template) error
	Delete(ctx context.Context, id uuid.UUID) error
	List(ctx context.Context, filter TemplateFilter, page CursorPagination) (*PaginatedResult[entity.Template], error)
	ListByCategory(ctx context.Context, categoryID uuid.UUID, page CursorPagination) (*PaginatedResult[entity.Template], error)
	IncrementUsageCount(ctx context.Context, id uuid.UUID) error
	SetTags(ctx context.Context, templateID uuid.UUID, tagIDs []uuid.UUID) error
	GetTags(ctx context.Context, templateID uuid.UUID) ([]entity.Tag, error)
}

type CategoryRepository interface {
	List(ctx context.Context) ([]entity.Category, error)
	GetByID(ctx context.Context, id uuid.UUID) (*entity.Category, error)
	GetBySlug(ctx context.Context, slug string) (*entity.Category, error)
}

type TagRepository interface {
	Create(ctx context.Context, tag *entity.Tag) error
	GetByName(ctx context.Context, name string) (*entity.Tag, error)
	GetOrCreate(ctx context.Context, name string) (*entity.Tag, error)
	ListPopular(ctx context.Context, limit int) ([]entity.Tag, error)
	Search(ctx context.Context, query string, limit int) ([]entity.Tag, error)
}

type StorageRepository interface {
	Upload(ctx context.Context, key string, reader io.Reader, contentType string, size int64) error
	Download(ctx context.Context, key string) (io.ReadCloser, error)
	Delete(ctx context.Context, key string) error
	GenerateSignedURL(ctx context.Context, key string, expiry time.Duration) (string, error)
}

type SearchRepository interface {
	IndexTemplate(ctx context.Context, template *entity.Template) error
	DeleteTemplate(ctx context.Context, id uuid.UUID) error
	SearchTemplates(ctx context.Context, query string, filter TemplateFilter, page CursorPagination) (*PaginatedResult[entity.Template], error)
}
