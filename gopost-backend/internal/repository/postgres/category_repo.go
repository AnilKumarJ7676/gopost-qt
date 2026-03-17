package postgres

import (
	"context"
	"errors"
	"fmt"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/database"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/google/uuid"
	"github.com/jackc/pgx/v5"
)

type categoryRepo struct {
	db *database.Postgres
}

func NewCategoryRepository(db *database.Postgres) repository.CategoryRepository {
	return &categoryRepo{db: db}
}

func (r *categoryRepo) List(ctx context.Context) ([]entity.Category, error) {
	rows, err := r.db.Pool.Query(ctx,
		`SELECT id, name, slug, description, icon_url, sort_order, is_active FROM categories WHERE is_active = true ORDER BY sort_order`)
	if err != nil {
		return nil, fmt.Errorf("listing categories: %w", err)
	}
	defer rows.Close()

	var cats []entity.Category
	for rows.Next() {
		var c entity.Category
		if err := rows.Scan(&c.ID, &c.Name, &c.Slug, &c.Description, &c.IconURL, &c.SortOrder, &c.IsActive); err != nil {
			return nil, fmt.Errorf("scanning category: %w", err)
		}
		cats = append(cats, c)
	}
	return cats, nil
}

func (r *categoryRepo) GetByID(ctx context.Context, id uuid.UUID) (*entity.Category, error) {
	var c entity.Category
	err := r.db.Pool.QueryRow(ctx,
		`SELECT id, name, slug, description, icon_url, sort_order, is_active FROM categories WHERE id = $1`, id,
	).Scan(&c.ID, &c.Name, &c.Slug, &c.Description, &c.IconURL, &c.SortOrder, &c.IsActive)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting category: %w", err)
	}
	return &c, nil
}

func (r *categoryRepo) GetBySlug(ctx context.Context, slug string) (*entity.Category, error) {
	var c entity.Category
	err := r.db.Pool.QueryRow(ctx,
		`SELECT id, name, slug, description, icon_url, sort_order, is_active FROM categories WHERE slug = $1`, slug,
	).Scan(&c.ID, &c.Name, &c.Slug, &c.Description, &c.IconURL, &c.SortOrder, &c.IsActive)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting category by slug: %w", err)
	}
	return &c, nil
}
