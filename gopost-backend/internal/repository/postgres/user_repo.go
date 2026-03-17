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

type userRepo struct {
	db *database.Postgres
}

func NewUserRepository(db *database.Postgres) repository.UserRepository {
	return &userRepo{db: db}
}

func (r *userRepo) Create(ctx context.Context, user *entity.User) error {
	query := `INSERT INTO users (id, email, password_hash, name, avatar_url, oauth_provider, oauth_id, device_fingerprint, created_at, updated_at)
		VALUES ($1, $2, $3, $4, $5, $6, $7, $8, NOW(), NOW())`

	if user.ID == uuid.Nil {
		user.ID = uuid.New()
	}

	_, err := r.db.Pool.Exec(ctx, query,
		user.ID, user.Email, user.PasswordHash, user.Name,
		user.AvatarURL, user.OAuthProvider, user.OAuthID, user.DeviceFingerprint,
	)
	if err != nil {
		return fmt.Errorf("creating user: %w", err)
	}
	return nil
}

func (r *userRepo) GetByID(ctx context.Context, id uuid.UUID) (*entity.User, error) {
	query := `SELECT id, email, password_hash, name, avatar_url, oauth_provider, oauth_id, device_fingerprint, created_at, updated_at, deleted_at
		FROM users WHERE id = $1 AND deleted_at IS NULL`

	var user entity.User
	err := r.db.Pool.QueryRow(ctx, query, id).Scan(
		&user.ID, &user.Email, &user.PasswordHash, &user.Name,
		&user.AvatarURL, &user.OAuthProvider, &user.OAuthID,
		&user.DeviceFingerprint, &user.CreatedAt, &user.UpdatedAt, &user.DeletedAt,
	)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting user by id: %w", err)
	}
	return &user, nil
}

func (r *userRepo) GetByEmail(ctx context.Context, email string) (*entity.User, error) {
	query := `SELECT id, email, password_hash, name, avatar_url, oauth_provider, oauth_id, device_fingerprint, created_at, updated_at, deleted_at
		FROM users WHERE email = $1 AND deleted_at IS NULL`

	var user entity.User
	err := r.db.Pool.QueryRow(ctx, query, email).Scan(
		&user.ID, &user.Email, &user.PasswordHash, &user.Name,
		&user.AvatarURL, &user.OAuthProvider, &user.OAuthID,
		&user.DeviceFingerprint, &user.CreatedAt, &user.UpdatedAt, &user.DeletedAt,
	)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting user by email: %w", err)
	}
	return &user, nil
}

func (r *userRepo) GetByOAuth(ctx context.Context, provider, oauthID string) (*entity.User, error) {
	query := `SELECT id, email, password_hash, name, avatar_url, oauth_provider, oauth_id, device_fingerprint, created_at, updated_at, deleted_at
		FROM users WHERE oauth_provider = $1 AND oauth_id = $2 AND deleted_at IS NULL`

	var user entity.User
	err := r.db.Pool.QueryRow(ctx, query, provider, oauthID).Scan(
		&user.ID, &user.Email, &user.PasswordHash, &user.Name,
		&user.AvatarURL, &user.OAuthProvider, &user.OAuthID,
		&user.DeviceFingerprint, &user.CreatedAt, &user.UpdatedAt, &user.DeletedAt,
	)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting user by oauth: %w", err)
	}
	return &user, nil
}

func (r *userRepo) Update(ctx context.Context, user *entity.User) error {
	query := `UPDATE users SET email = $1, name = $2, avatar_url = $3, updated_at = NOW() WHERE id = $4 AND deleted_at IS NULL`
	_, err := r.db.Pool.Exec(ctx, query, user.Email, user.Name, user.AvatarURL, user.ID)
	if err != nil {
		return fmt.Errorf("updating user: %w", err)
	}
	return nil
}
