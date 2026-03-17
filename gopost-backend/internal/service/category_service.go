package service

import (
	"context"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/google/uuid"
)

type CategoryService interface {
	List(ctx context.Context) ([]entity.Category, error)
	GetByID(ctx context.Context, id uuid.UUID) (*entity.Category, error)
}

type categoryService struct {
	repo repository.CategoryRepository
}

func NewCategoryService(repo repository.CategoryRepository) CategoryService {
	return &categoryService{repo: repo}
}

func (s *categoryService) List(ctx context.Context) ([]entity.Category, error) {
	return s.repo.List(ctx)
}

func (s *categoryService) GetByID(ctx context.Context, id uuid.UUID) (*entity.Category, error) {
	cat, err := s.repo.GetByID(ctx, id)
	if err != nil {
		return nil, err
	}
	if cat == nil {
		return nil, &AppError{Code: "NOT_FOUND", Message: "Category not found", Status: 404}
	}
	return cat, nil
}
