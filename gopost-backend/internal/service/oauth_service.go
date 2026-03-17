package service

import (
	"context"
	"fmt"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/gopost-app/gopost-backend/pkg/jwt"
	"github.com/google/uuid"
)

type OAuthUserInfo struct {
	Provider string
	ID       string
	Email    string
	Name     string
	Avatar   string
}

type OAuthProvider interface {
	ValidateToken(ctx context.Context, idToken string) (*OAuthUserInfo, error)
	ProviderName() string
}

type OAuthService interface {
	Authenticate(ctx context.Context, provider string, idToken string) (*AuthResponse, error)
}

type oauthService struct {
	providers   map[string]OAuthProvider
	userRepo    repository.UserRepository
	sessionRepo repository.SessionRepository
	roleRepo    repository.RoleRepository
	jwtManager  *jwt.Manager
	refreshTTL  int
}

func NewOAuthService(
	userRepo repository.UserRepository,
	sessionRepo repository.SessionRepository,
	roleRepo repository.RoleRepository,
	jwtManager *jwt.Manager,
	refreshTTLDays int,
	providers ...OAuthProvider,
) OAuthService {
	pm := make(map[string]OAuthProvider)
	for _, p := range providers {
		pm[p.ProviderName()] = p
	}
	return &oauthService{
		providers:   pm,
		userRepo:    userRepo,
		sessionRepo: sessionRepo,
		roleRepo:    roleRepo,
		jwtManager:  jwtManager,
		refreshTTL:  refreshTTLDays,
	}
}

func (s *oauthService) Authenticate(ctx context.Context, provider string, idToken string) (*AuthResponse, error) {
	p, ok := s.providers[provider]
	if !ok {
		return nil, &AppError{Code: "INVALID_PROVIDER", Message: fmt.Sprintf("Unsupported OAuth provider: %s", provider), Status: 400}
	}

	info, err := p.ValidateToken(ctx, idToken)
	if err != nil {
		return nil, &AppError{Code: "OAUTH_FAILED", Message: "OAuth validation failed", Status: 401}
	}

	user, err := s.userRepo.GetByOAuth(ctx, info.Provider, info.ID)
	if err != nil {
		return nil, fmt.Errorf("looking up oauth user: %w", err)
	}

	if user == nil {
		user, err = s.userRepo.GetByEmail(ctx, info.Email)
		if err != nil {
			return nil, fmt.Errorf("looking up user by email: %w", err)
		}
	}

	if user == nil {
		user = &entity.User{
			ID:            uuid.New(),
			Email:         info.Email,
			Name:          info.Name,
			AvatarURL:     info.Avatar,
			OAuthProvider: info.Provider,
			OAuthID:       info.ID,
		}
		if err := s.userRepo.Create(ctx, user); err != nil {
			return nil, fmt.Errorf("creating oauth user: %w", err)
		}

		role, _ := s.roleRepo.GetByName(ctx, entity.RoleUser)
		if role != nil {
			_ = s.roleRepo.AssignRole(ctx, user.ID, role.ID)
		}
	}

	roles, _ := s.roleRepo.GetUserRoles(ctx, user.ID)
	roleName := entity.RoleUser
	if len(roles) > 0 {
		roleName = roles[0].Name
	}

	authSvc := NewAuthService(s.userRepo, s.sessionRepo, s.roleRepo, s.jwtManager, s.refreshTTL)
	return authSvc.(*authService).issueTokens(ctx, user, roleName, nil)
}
