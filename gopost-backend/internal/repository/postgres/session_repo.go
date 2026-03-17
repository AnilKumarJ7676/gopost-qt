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

type sessionRepo struct {
	db *database.Postgres
}

func NewSessionRepository(db *database.Postgres) repository.SessionRepository {
	return &sessionRepo{db: db}
}

func (r *sessionRepo) Create(ctx context.Context, session *entity.Session) error {
	query := `INSERT INTO sessions (id, user_id, refresh_token_hash, device_fingerprint, ip_address, user_agent, expires_at, created_at)
		VALUES ($1, $2, $3, $4, $5, $6, $7, NOW())`

	if session.ID == uuid.Nil {
		session.ID = uuid.New()
	}

	_, err := r.db.Pool.Exec(ctx, query,
		session.ID, session.UserID, session.RefreshTokenHash,
		session.DeviceFingerprint, session.IPAddress, session.UserAgent, session.ExpiresAt,
	)
	if err != nil {
		return fmt.Errorf("creating session: %w", err)
	}
	return nil
}

func (r *sessionRepo) GetByRefreshTokenHash(ctx context.Context, hash string) (*entity.Session, error) {
	query := `SELECT id, user_id, refresh_token_hash, device_fingerprint, ip_address, user_agent, expires_at, created_at
		FROM sessions WHERE refresh_token_hash = $1`

	var s entity.Session
	err := r.db.Pool.QueryRow(ctx, query, hash).Scan(
		&s.ID, &s.UserID, &s.RefreshTokenHash, &s.DeviceFingerprint,
		&s.IPAddress, &s.UserAgent, &s.ExpiresAt, &s.CreatedAt,
	)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting session by refresh token: %w", err)
	}
	return &s, nil
}

func (r *sessionRepo) DeleteByID(ctx context.Context, id uuid.UUID) error {
	_, err := r.db.Pool.Exec(ctx, `DELETE FROM sessions WHERE id = $1`, id)
	if err != nil {
		return fmt.Errorf("deleting session: %w", err)
	}
	return nil
}

func (r *sessionRepo) DeleteAllByUserID(ctx context.Context, userID uuid.UUID) error {
	_, err := r.db.Pool.Exec(ctx, `DELETE FROM sessions WHERE user_id = $1`, userID)
	if err != nil {
		return fmt.Errorf("deleting all user sessions: %w", err)
	}
	return nil
}
