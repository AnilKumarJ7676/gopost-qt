package postgres

import (
	"context"
	"errors"
	"fmt"
	"strings"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/database"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/google/uuid"
	"github.com/jackc/pgx/v5"
)

type tagRepo struct {
	db *database.Postgres
}

func NewTagRepository(db *database.Postgres) repository.TagRepository {
	return &tagRepo{db: db}
}

func (r *tagRepo) Create(ctx context.Context, tag *entity.Tag) error {
	if tag.ID == uuid.Nil {
		tag.ID = uuid.New()
	}
	if tag.Slug == "" {
		tag.Slug = strings.ToLower(strings.ReplaceAll(tag.Name, " ", "-"))
	}
	_, err := r.db.Pool.Exec(ctx, `INSERT INTO tags (id, name, slug) VALUES ($1, $2, $3)`, tag.ID, tag.Name, tag.Slug)
	if err != nil {
		return fmt.Errorf("creating tag: %w", err)
	}
	return nil
}

func (r *tagRepo) GetByName(ctx context.Context, name string) (*entity.Tag, error) {
	var tag entity.Tag
	err := r.db.Pool.QueryRow(ctx, `SELECT id, name, slug FROM tags WHERE name = $1`, name).Scan(&tag.ID, &tag.Name, &tag.Slug)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting tag: %w", err)
	}
	return &tag, nil
}

func (r *tagRepo) GetOrCreate(ctx context.Context, name string) (*entity.Tag, error) {
	tag, err := r.GetByName(ctx, name)
	if err != nil {
		return nil, err
	}
	if tag != nil {
		return tag, nil
	}
	tag = &entity.Tag{Name: name}
	if err := r.Create(ctx, tag); err != nil {
		return nil, err
	}
	return tag, nil
}

func (r *tagRepo) ListPopular(ctx context.Context, limit int) ([]entity.Tag, error) {
	if limit <= 0 {
		limit = 20
	}
	rows, err := r.db.Pool.Query(ctx,
		`SELECT t.id, t.name, t.slug FROM tags t
		JOIN template_tags tt ON tt.tag_id = t.id
		GROUP BY t.id, t.name, t.slug
		ORDER BY COUNT(tt.template_id) DESC LIMIT $1`, limit)
	if err != nil {
		return nil, fmt.Errorf("listing popular tags: %w", err)
	}
	defer rows.Close()

	var tags []entity.Tag
	for rows.Next() {
		var tag entity.Tag
		if err := rows.Scan(&tag.ID, &tag.Name, &tag.Slug); err != nil {
			return nil, fmt.Errorf("scanning tag: %w", err)
		}
		tags = append(tags, tag)
	}
	return tags, nil
}

func (r *tagRepo) Search(ctx context.Context, query string, limit int) ([]entity.Tag, error) {
	if limit <= 0 {
		limit = 20
	}
	rows, err := r.db.Pool.Query(ctx,
		`SELECT id, name, slug FROM tags WHERE name ILIKE $1 ORDER BY name LIMIT $2`,
		"%"+query+"%", limit)
	if err != nil {
		return nil, fmt.Errorf("searching tags: %w", err)
	}
	defer rows.Close()

	var tags []entity.Tag
	for rows.Next() {
		var tag entity.Tag
		if err := rows.Scan(&tag.ID, &tag.Name, &tag.Slug); err != nil {
			return nil, fmt.Errorf("scanning tag: %w", err)
		}
		tags = append(tags, tag)
	}
	return tags, nil
}
