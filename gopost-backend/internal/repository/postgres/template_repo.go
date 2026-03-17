package postgres

import (
	"context"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"strings"
	"time"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/database"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/google/uuid"
	"github.com/jackc/pgx/v5"
)

type templateRepo struct {
	db *database.Postgres
}

func NewTemplateRepository(db *database.Postgres) repository.TemplateRepository {
	return &templateRepo{db: db}
}

func marshalEditableFields(v interface{}) ([]byte, error) {
	if v == nil {
		return []byte("[]"), nil
	}
	return json.Marshal(v)
}

func (r *templateRepo) Create(ctx context.Context, t *entity.Template) error {
	if t.ID == uuid.Nil {
		t.ID = uuid.New()
	}
	editableJSON, err := marshalEditableFields(t.EditableFields)
	if err != nil {
		return fmt.Errorf("marshaling editable fields: %w", err)
	}
	query := `INSERT INTO templates (id, name, description, type, category_id, creator_id, status, storage_key, thumbnail_url, preview_url, width, height, duration_ms, layer_count, editable_fields, usage_count, is_premium, version, created_at, updated_at)
		VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,NOW(),NOW())`
	_, err = r.db.Pool.Exec(ctx, query,
		t.ID, t.Name, t.Description, t.Type, t.CategoryID, t.CreatorID, t.Status,
		t.StorageKey, t.ThumbnailURL, t.PreviewURL, t.Width, t.Height, t.DurationMs,
		t.LayerCount, editableJSON, t.UsageCount, t.IsPremium, t.Version)
	if err != nil {
		return fmt.Errorf("creating template: %w", err)
	}
	return nil
}

func (r *templateRepo) GetByID(ctx context.Context, id uuid.UUID) (*entity.Template, error) {
	query := `SELECT t.id, t.name, t.description, t.type, t.category_id, t.creator_id, t.status,
		t.storage_key, t.thumbnail_url, t.preview_url, t.width, t.height, t.duration_ms,
		t.layer_count, t.editable_fields, t.usage_count, t.is_premium, t.version,
		t.created_at, t.updated_at, t.published_at,
		c.id, c.name, c.slug
		FROM templates t
		LEFT JOIN categories c ON c.id = t.category_id
		WHERE t.id = $1`

	var t entity.Template
	var cat entity.Category
	var catID *uuid.UUID
	var editableJSON []byte
	err := r.db.Pool.QueryRow(ctx, query, id).Scan(
		&t.ID, &t.Name, &t.Description, &t.Type, &t.CategoryID, &t.CreatorID, &t.Status,
		&t.StorageKey, &t.ThumbnailURL, &t.PreviewURL, &t.Width, &t.Height, &t.DurationMs,
		&t.LayerCount, &editableJSON, &t.UsageCount, &t.IsPremium, &t.Version,
		&t.CreatedAt, &t.UpdatedAt, &t.PublishedAt,
		&catID, &cat.Name, &cat.Slug)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting template: %w", err)
	}
	if len(editableJSON) > 0 {
		_ = json.Unmarshal(editableJSON, &t.EditableFields)
	}
	if catID != nil {
		cat.ID = *catID
		t.Category = &cat
	}
	return &t, nil
}

func (r *templateRepo) Update(ctx context.Context, t *entity.Template) error {
	editableJSON, err := marshalEditableFields(t.EditableFields)
	if err != nil {
		return fmt.Errorf("marshaling editable fields: %w", err)
	}
	query := `UPDATE templates SET name=$1, description=$2, type=$3, category_id=$4, status=$5,
		storage_key=$6, thumbnail_url=$7, preview_url=$8, width=$9, height=$10, duration_ms=$11,
		layer_count=$12, editable_fields=$13, is_premium=$14, version=$15, updated_at=NOW(),
		published_at=$16
		WHERE id=$17`
	_, err = r.db.Pool.Exec(ctx, query,
		t.Name, t.Description, t.Type, t.CategoryID, t.Status,
		t.StorageKey, t.ThumbnailURL, t.PreviewURL, t.Width, t.Height, t.DurationMs,
		t.LayerCount, editableJSON, t.IsPremium, t.Version, t.PublishedAt, t.ID)
	if err != nil {
		return fmt.Errorf("updating template: %w", err)
	}
	return nil
}

func (r *templateRepo) Delete(ctx context.Context, id uuid.UUID) error {
	_, err := r.db.Pool.Exec(ctx, `DELETE FROM templates WHERE id = $1`, id)
	if err != nil {
		return fmt.Errorf("deleting template: %w", err)
	}
	return nil
}

func (r *templateRepo) List(ctx context.Context, filter repository.TemplateFilter, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error) {
	limit := page.Limit
	if limit <= 0 || limit > 50 {
		limit = 20
	}

	var conditions []string
	var args []interface{}
	argIdx := 1

	if filter.Type != "" {
		conditions = append(conditions, fmt.Sprintf("t.type = $%d", argIdx))
		args = append(args, filter.Type)
		argIdx++
	}
	if filter.Status != "" {
		conditions = append(conditions, fmt.Sprintf("t.status = $%d", argIdx))
		args = append(args, filter.Status)
		argIdx++
	}
	if filter.CategoryID != nil {
		conditions = append(conditions, fmt.Sprintf("t.category_id = $%d", argIdx))
		args = append(args, *filter.CategoryID)
		argIdx++
	}
	if filter.IsPremium != nil {
		conditions = append(conditions, fmt.Sprintf("t.is_premium = $%d", argIdx))
		args = append(args, *filter.IsPremium)
		argIdx++
	}
	if filter.CreatorID != nil {
		conditions = append(conditions, fmt.Sprintf("t.creator_id = $%d", argIdx))
		args = append(args, *filter.CreatorID)
		argIdx++
	}
	if filter.Query != "" {
		conditions = append(conditions, fmt.Sprintf("(t.name ILIKE $%d OR t.description ILIKE $%d)", argIdx, argIdx))
		args = append(args, "%"+filter.Query+"%")
		argIdx++
	}

	if page.Cursor != "" {
		decoded, _ := base64.StdEncoding.DecodeString(page.Cursor)
		cursorTime, err := time.Parse(time.RFC3339Nano, string(decoded))
		if err == nil {
			conditions = append(conditions, fmt.Sprintf("t.created_at < $%d", argIdx))
			args = append(args, cursorTime)
			argIdx++
		}
	}

	where := ""
	if len(conditions) > 0 {
		where = "WHERE " + strings.Join(conditions, " AND ")
	}

	orderBy := "t.created_at DESC"
	switch filter.Sort {
	case "popular":
		orderBy = "t.usage_count DESC, t.created_at DESC"
	case "newest":
		orderBy = "t.published_at DESC NULLS LAST, t.created_at DESC"
	}

	countQuery := fmt.Sprintf("SELECT COUNT(*) FROM templates t %s", where)
	var total int64
	_ = r.db.Pool.QueryRow(ctx, countQuery, args...).Scan(&total)

	query := fmt.Sprintf(`SELECT t.id, t.name, t.description, t.type, t.category_id, t.status,
		t.thumbnail_url, t.preview_url, t.width, t.height, t.duration_ms,
		t.usage_count, t.is_premium, t.version, t.created_at, t.updated_at, t.published_at
		FROM templates t %s ORDER BY %s LIMIT $%d`, where, orderBy, argIdx)
	args = append(args, limit+1)

	rows, err := r.db.Pool.Query(ctx, query, args...)
	if err != nil {
		return nil, fmt.Errorf("listing templates: %w", err)
	}
	defer rows.Close()

	var items []entity.Template
	for rows.Next() {
		var t entity.Template
		if err := rows.Scan(
			&t.ID, &t.Name, &t.Description, &t.Type, &t.CategoryID, &t.Status,
			&t.ThumbnailURL, &t.PreviewURL, &t.Width, &t.Height, &t.DurationMs,
			&t.UsageCount, &t.IsPremium, &t.Version, &t.CreatedAt, &t.UpdatedAt, &t.PublishedAt); err != nil {
			return nil, fmt.Errorf("scanning template: %w", err)
		}
		items = append(items, t)
	}

	hasMore := len(items) > limit
	if hasMore {
		items = items[:limit]
	}

	var nextCursor string
	if hasMore && len(items) > 0 {
		last := items[len(items)-1]
		nextCursor = base64.StdEncoding.EncodeToString([]byte(last.CreatedAt.Format(time.RFC3339Nano)))
	}

	return &repository.PaginatedResult[entity.Template]{
		Items: items, NextCursor: nextCursor, HasMore: hasMore, Total: total,
	}, nil
}

func (r *templateRepo) ListByCategory(ctx context.Context, categoryID uuid.UUID, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error) {
	return r.List(ctx, repository.TemplateFilter{CategoryID: &categoryID, Status: "published"}, page)
}

func (r *templateRepo) IncrementUsageCount(ctx context.Context, id uuid.UUID) error {
	_, err := r.db.Pool.Exec(ctx, `UPDATE templates SET usage_count = usage_count + 1 WHERE id = $1`, id)
	if err != nil {
		return fmt.Errorf("incrementing usage count: %w", err)
	}
	return nil
}

func (r *templateRepo) SetTags(ctx context.Context, templateID uuid.UUID, tagIDs []uuid.UUID) error {
	tx, err := r.db.Pool.Begin(ctx)
	if err != nil {
		return fmt.Errorf("beginning transaction: %w", err)
	}
	defer tx.Rollback(ctx)

	_, err = tx.Exec(ctx, `DELETE FROM template_tags WHERE template_id = $1`, templateID)
	if err != nil {
		return fmt.Errorf("clearing tags: %w", err)
	}

	for _, tagID := range tagIDs {
		_, err = tx.Exec(ctx, `INSERT INTO template_tags (template_id, tag_id) VALUES ($1, $2)`, templateID, tagID)
		if err != nil {
			return fmt.Errorf("setting tag: %w", err)
		}
	}

	return tx.Commit(ctx)
}

func (r *templateRepo) GetTags(ctx context.Context, templateID uuid.UUID) ([]entity.Tag, error) {
	rows, err := r.db.Pool.Query(ctx,
		`SELECT t.id, t.name, t.slug FROM tags t JOIN template_tags tt ON tt.tag_id = t.id WHERE tt.template_id = $1`, templateID)
	if err != nil {
		return nil, fmt.Errorf("getting tags: %w", err)
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
