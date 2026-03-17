package service

import (
	"context"
	"fmt"
	"time"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/google/uuid"
)

type TemplateService interface {
	Create(ctx context.Context, req CreateTemplateRequest) (*entity.Template, error)
	GetByID(ctx context.Context, id uuid.UUID) (*entity.Template, error)
	Update(ctx context.Context, id uuid.UUID, req UpdateTemplateRequest) (*entity.Template, error)
	Delete(ctx context.Context, id uuid.UUID) error
	List(ctx context.Context, filter repository.TemplateFilter, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error)
	Search(ctx context.Context, query string, filter repository.TemplateFilter, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error)
	ListByCategory(ctx context.Context, categoryID uuid.UUID, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error)
	Publish(ctx context.Context, id uuid.UUID) (*entity.Template, error)
	IncrementUsage(ctx context.Context, id uuid.UUID) error
}

type CreateTemplateRequest struct {
	Name           string         `json:"name"`
	Description    string         `json:"description"`
	Type           string         `json:"type"`
	CategoryID     *uuid.UUID     `json:"category_id"`
	CreatorID      *uuid.UUID     `json:"creator_id"`
	Width          int            `json:"width"`
	Height         int            `json:"height"`
	DurationMs     int            `json:"duration_ms"`
	LayerCount     int            `json:"layer_count"`
	IsPremium      bool           `json:"is_premium"`
	Tags           []string       `json:"tags"`
	StorageKey     string         `json:"storage_key,omitempty"`
	EditableFields interface{}    `json:"editable_fields,omitempty"`
}

type UpdateTemplateRequest struct {
	Name        *string    `json:"name,omitempty"`
	Description *string    `json:"description,omitempty"`
	CategoryID  *uuid.UUID `json:"category_id,omitempty"`
	IsPremium   *bool      `json:"is_premium,omitempty"`
	Tags        []string   `json:"tags,omitempty"`
}

// CDN is an optional abstraction for CDN cache operations.
type CDN interface {
	PurgeTemplate(ctx context.Context, storageKey string) error
	WarmTemplate(ctx context.Context, storageKey string)
	TemplateURL(storageKey string) string
}

type templateService struct {
	templateRepo repository.TemplateRepository
	categoryRepo repository.CategoryRepository
	tagRepo      repository.TagRepository
	searchRepo   repository.SearchRepository
	cdn          CDN
}

func NewTemplateService(
	templateRepo repository.TemplateRepository,
	categoryRepo repository.CategoryRepository,
	tagRepo repository.TagRepository,
	searchRepo repository.SearchRepository,
	cdn CDN,
) TemplateService {
	return &templateService{
		templateRepo: templateRepo,
		categoryRepo: categoryRepo,
		tagRepo:      tagRepo,
		searchRepo:   searchRepo,
		cdn:          cdn,
	}
}

func (s *templateService) Create(ctx context.Context, req CreateTemplateRequest) (*entity.Template, error) {
	if req.Name == "" {
		return nil, &AppError{Code: "VALIDATION_ERROR", Message: "Name is required", Status: 400}
	}
	if req.Type != string(entity.TemplateTypeVideo) && req.Type != string(entity.TemplateTypeImage) {
		return nil, &AppError{Code: "VALIDATION_ERROR", Message: "Type must be 'video' or 'image'", Status: 400}
	}

	t := &entity.Template{
		ID:             uuid.New(),
		Name:           req.Name,
		Description:    req.Description,
		Type:           entity.TemplateType(req.Type),
		CategoryID:     req.CategoryID,
		CreatorID:      req.CreatorID,
		Status:         entity.TemplateStatusDraft,
		StorageKey:     req.StorageKey,
		Width:          req.Width,
		Height:         req.Height,
		DurationMs:     req.DurationMs,
		LayerCount:     req.LayerCount,
		EditableFields: req.EditableFields,
		IsPremium:      req.IsPremium,
		Version:        1,
	}

	if err := s.templateRepo.Create(ctx, t); err != nil {
		return nil, fmt.Errorf("creating template: %w", err)
	}

	if len(req.Tags) > 0 {
		var tagIDs []uuid.UUID
		for _, tagName := range req.Tags {
			tag, err := s.tagRepo.GetOrCreate(ctx, tagName)
			if err != nil {
				continue
			}
			tagIDs = append(tagIDs, tag.ID)
		}
		_ = s.templateRepo.SetTags(ctx, t.ID, tagIDs)
	}

	if s.searchRepo != nil {
		_ = s.searchRepo.IndexTemplate(ctx, t)
	}

	return t, nil
}

func (s *templateService) GetByID(ctx context.Context, id uuid.UUID) (*entity.Template, error) {
	t, err := s.templateRepo.GetByID(ctx, id)
	if err != nil {
		return nil, fmt.Errorf("getting template: %w", err)
	}
	if t == nil {
		return nil, &AppError{Code: "NOT_FOUND", Message: "Template not found", Status: 404}
	}

	tags, _ := s.templateRepo.GetTags(ctx, id)
	t.Tags = tags
	return t, nil
}

func (s *templateService) Update(ctx context.Context, id uuid.UUID, req UpdateTemplateRequest) (*entity.Template, error) {
	t, err := s.templateRepo.GetByID(ctx, id)
	if err != nil {
		return nil, fmt.Errorf("getting template: %w", err)
	}
	if t == nil {
		return nil, &AppError{Code: "NOT_FOUND", Message: "Template not found", Status: 404}
	}

	if req.Name != nil {
		t.Name = *req.Name
	}
	if req.Description != nil {
		t.Description = *req.Description
	}
	if req.CategoryID != nil {
		t.CategoryID = req.CategoryID
	}
	if req.IsPremium != nil {
		t.IsPremium = *req.IsPremium
	}

	if err := s.templateRepo.Update(ctx, t); err != nil {
		return nil, fmt.Errorf("updating template: %w", err)
	}

	if req.Tags != nil {
		var tagIDs []uuid.UUID
		for _, tagName := range req.Tags {
			tag, err := s.tagRepo.GetOrCreate(ctx, tagName)
			if err != nil {
				continue
			}
			tagIDs = append(tagIDs, tag.ID)
		}
		_ = s.templateRepo.SetTags(ctx, t.ID, tagIDs)
	}

	if s.searchRepo != nil {
		_ = s.searchRepo.IndexTemplate(ctx, t)
	}
	if s.cdn != nil && t.StorageKey != "" {
		_ = s.cdn.PurgeTemplate(ctx, t.StorageKey)
	}

	return t, nil
}

func (s *templateService) Delete(ctx context.Context, id uuid.UUID) error {
	t, err := s.templateRepo.GetByID(ctx, id)
	if err != nil {
		return fmt.Errorf("getting template: %w", err)
	}
	if t == nil {
		return &AppError{Code: "NOT_FOUND", Message: "Template not found", Status: 404}
	}

	if err := s.templateRepo.Delete(ctx, id); err != nil {
		return fmt.Errorf("deleting template: %w", err)
	}

	if s.searchRepo != nil {
		_ = s.searchRepo.DeleteTemplate(ctx, id)
	}
	if s.cdn != nil && t.StorageKey != "" {
		_ = s.cdn.PurgeTemplate(ctx, t.StorageKey)
	}

	return nil
}

func (s *templateService) List(ctx context.Context, filter repository.TemplateFilter, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error) {
	return s.templateRepo.List(ctx, filter, page)
}

func (s *templateService) Search(ctx context.Context, query string, filter repository.TemplateFilter, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error) {
	if s.searchRepo != nil {
		return s.searchRepo.SearchTemplates(ctx, query, filter, page)
	}
	filter.Query = query
	return s.templateRepo.List(ctx, filter, page)
}

func (s *templateService) ListByCategory(ctx context.Context, categoryID uuid.UUID, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error) {
	return s.templateRepo.ListByCategory(ctx, categoryID, page)
}

func (s *templateService) Publish(ctx context.Context, id uuid.UUID) (*entity.Template, error) {
	t, err := s.templateRepo.GetByID(ctx, id)
	if err != nil {
		return nil, fmt.Errorf("getting template: %w", err)
	}
	if t == nil {
		return nil, &AppError{Code: "NOT_FOUND", Message: "Template not found", Status: 404}
	}

	now := time.Now()
	t.Status = entity.TemplateStatusPublished
	t.PublishedAt = &now

	if err := s.templateRepo.Update(ctx, t); err != nil {
		return nil, fmt.Errorf("publishing template: %w", err)
	}

	if s.searchRepo != nil {
		_ = s.searchRepo.IndexTemplate(ctx, t)
	}
	if s.cdn != nil && t.StorageKey != "" {
		go s.cdn.WarmTemplate(context.Background(), t.StorageKey)
	}

	return t, nil
}

func (s *templateService) IncrementUsage(ctx context.Context, id uuid.UUID) error {
	return s.templateRepo.IncrementUsageCount(ctx, id)
}
