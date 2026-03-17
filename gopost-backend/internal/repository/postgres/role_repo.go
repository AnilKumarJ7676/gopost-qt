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

type roleRepo struct {
	db *database.Postgres
}

func NewRoleRepository(db *database.Postgres) repository.RoleRepository {
	return &roleRepo{db: db}
}

func (r *roleRepo) GetByName(ctx context.Context, name string) (*entity.Role, error) {
	var role entity.Role
	err := r.db.Pool.QueryRow(ctx,
		`SELECT id, name, description FROM roles WHERE name = $1`, name,
	).Scan(&role.ID, &role.Name, &role.Description)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("getting role: %w", err)
	}
	return &role, nil
}

func (r *roleRepo) GetUserRoles(ctx context.Context, userID uuid.UUID) ([]entity.Role, error) {
	rows, err := r.db.Pool.Query(ctx,
		`SELECT r.id, r.name, r.description FROM roles r
		 JOIN user_roles ur ON ur.role_id = r.id WHERE ur.user_id = $1`, userID)
	if err != nil {
		return nil, fmt.Errorf("getting user roles: %w", err)
	}
	defer rows.Close()

	var roles []entity.Role
	for rows.Next() {
		var role entity.Role
		if err := rows.Scan(&role.ID, &role.Name, &role.Description); err != nil {
			return nil, fmt.Errorf("scanning role: %w", err)
		}
		roles = append(roles, role)
	}
	return roles, nil
}

func (r *roleRepo) AssignRole(ctx context.Context, userID, roleID uuid.UUID) error {
	_, err := r.db.Pool.Exec(ctx,
		`INSERT INTO user_roles (id, user_id, role_id) VALUES ($1, $2, $3) ON CONFLICT (user_id, role_id) DO NOTHING`,
		uuid.New(), userID, roleID)
	if err != nil {
		return fmt.Errorf("assigning role: %w", err)
	}
	return nil
}
