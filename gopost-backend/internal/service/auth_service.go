package service

import (
	"context"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"net/http"
	"time"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/gopost-app/gopost-backend/pkg/jwt"
	"github.com/google/uuid"
	"golang.org/x/crypto/bcrypt"
)

type AuthService interface {
	Register(ctx context.Context, req RegisterRequest) (*AuthResponse, error)
	Login(ctx context.Context, req LoginRequest) (*AuthResponse, error)
	RefreshToken(ctx context.Context, refreshToken string, req *http.Request) (*AuthResponse, error)
	Logout(ctx context.Context, refreshToken string) error
}

type RegisterRequest struct {
	Name     string `json:"name"`
	Email    string `json:"email"`
	Password string `json:"password"`
}

type LoginRequest struct {
	Email    string `json:"email"`
	Password string `json:"password"`
}

type AuthResponse struct {
	AccessToken  string       `json:"access_token"`
	RefreshToken string       `json:"refresh_token"`
	ExpiresIn    int64        `json:"expires_in"`
	User         *entity.User `json:"user"`
}

type authService struct {
	userRepo    repository.UserRepository
	sessionRepo repository.SessionRepository
	roleRepo    repository.RoleRepository
	jwtManager  *jwt.Manager
	refreshTTL  time.Duration
}

func NewAuthService(
	userRepo repository.UserRepository,
	sessionRepo repository.SessionRepository,
	roleRepo repository.RoleRepository,
	jwtManager *jwt.Manager,
	refreshTTLDays int,
) AuthService {
	return &authService{
		userRepo:    userRepo,
		sessionRepo: sessionRepo,
		roleRepo:    roleRepo,
		jwtManager:  jwtManager,
		refreshTTL:  time.Duration(refreshTTLDays) * 24 * time.Hour,
	}
}

func (s *authService) Register(ctx context.Context, req RegisterRequest) (*AuthResponse, error) {
	existing, err := s.userRepo.GetByEmail(ctx, req.Email)
	if err != nil {
		return nil, fmt.Errorf("checking existing user: %w", err)
	}
	if existing != nil {
		return nil, &AppError{Code: "EMAIL_EXISTS", Message: "Email already registered", Status: 409}
	}

	hash, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
	if err != nil {
		return nil, fmt.Errorf("hashing password: %w", err)
	}

	user := &entity.User{
		ID:           uuid.New(),
		Email:        req.Email,
		PasswordHash: string(hash),
		Name:         req.Name,
	}

	if err := s.userRepo.Create(ctx, user); err != nil {
		return nil, fmt.Errorf("creating user: %w", err)
	}

	role, err := s.roleRepo.GetByName(ctx, entity.RoleUser)
	if err != nil {
		return nil, fmt.Errorf("getting default role: %w", err)
	}
	if role != nil {
		_ = s.roleRepo.AssignRole(ctx, user.ID, role.ID)
	}

	return s.issueTokens(ctx, user, entity.RoleUser, nil)
}

func (s *authService) Login(ctx context.Context, req LoginRequest) (*AuthResponse, error) {
	user, err := s.userRepo.GetByEmail(ctx, req.Email)
	if err != nil {
		return nil, fmt.Errorf("finding user: %w", err)
	}
	if user == nil {
		return nil, &AppError{Code: "INVALID_CREDENTIALS", Message: "Invalid email or password", Status: 401}
	}

	if err := bcrypt.CompareHashAndPassword([]byte(user.PasswordHash), []byte(req.Password)); err != nil {
		return nil, &AppError{Code: "INVALID_CREDENTIALS", Message: "Invalid email or password", Status: 401}
	}

	roles, _ := s.roleRepo.GetUserRoles(ctx, user.ID)
	roleName := entity.RoleUser
	if len(roles) > 0 {
		roleName = roles[0].Name
	}

	return s.issueTokens(ctx, user, roleName, nil)
}

func (s *authService) RefreshToken(ctx context.Context, refreshToken string, req *http.Request) (*AuthResponse, error) {
	claims, err := s.jwtManager.Validate(refreshToken)
	if err != nil {
		return nil, &AppError{Code: "INVALID_TOKEN", Message: "Invalid or expired refresh token", Status: 401}
	}

	tokenHash := hashToken(refreshToken)
	session, err := s.sessionRepo.GetByRefreshTokenHash(ctx, tokenHash)
	if err != nil {
		return nil, fmt.Errorf("finding session: %w", err)
	}

	if session == nil {
		userID, _ := uuid.Parse(claims.UserID)
		_ = s.sessionRepo.DeleteAllByUserID(ctx, userID)
		return nil, &AppError{Code: "TOKEN_REUSE", Message: "Refresh token reuse detected, all sessions revoked", Status: 401}
	}

	if err := s.sessionRepo.DeleteByID(ctx, session.ID); err != nil {
		return nil, fmt.Errorf("deleting old session: %w", err)
	}

	userID, _ := uuid.Parse(claims.UserID)
	user, err := s.userRepo.GetByID(ctx, userID)
	if err != nil || user == nil {
		return nil, &AppError{Code: "USER_NOT_FOUND", Message: "User not found", Status: 401}
	}

	return s.issueTokens(ctx, user, claims.Role, req)
}

func (s *authService) Logout(ctx context.Context, refreshToken string) error {
	tokenHash := hashToken(refreshToken)
	session, err := s.sessionRepo.GetByRefreshTokenHash(ctx, tokenHash)
	if err != nil {
		return fmt.Errorf("finding session: %w", err)
	}
	if session != nil {
		return s.sessionRepo.DeleteByID(ctx, session.ID)
	}
	return nil
}

func (s *authService) issueTokens(ctx context.Context, user *entity.User, role string, req *http.Request) (*AuthResponse, error) {
	accessToken, err := s.jwtManager.GenerateAccessToken(user.ID.String(), role)
	if err != nil {
		return nil, fmt.Errorf("generating access token: %w", err)
	}

	refreshToken, err := s.jwtManager.GenerateRefreshToken(user.ID.String(), role)
	if err != nil {
		return nil, fmt.Errorf("generating refresh token: %w", err)
	}

	session := &entity.Session{
		ID:               uuid.New(),
		UserID:           user.ID,
		RefreshTokenHash: hashToken(refreshToken),
		ExpiresAt:        time.Now().Add(s.refreshTTL),
	}

	if req != nil {
		session.IPAddress = req.RemoteAddr
		session.UserAgent = req.UserAgent()
	}

	if err := s.sessionRepo.Create(ctx, session); err != nil {
		return nil, fmt.Errorf("creating session: %w", err)
	}

	return &AuthResponse{
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		ExpiresIn:    900,
		User:         user,
	}, nil
}

func hashToken(token string) string {
	h := sha256.Sum256([]byte(token))
	return hex.EncodeToString(h[:])
}
