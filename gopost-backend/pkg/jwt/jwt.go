package jwt

import (
	"fmt"
	"time"

	jwtgo "github.com/golang-jwt/jwt/v5"
)

type Claims struct {
	UserID string `json:"uid"`
	Role   string `json:"role"`
	jwtgo.RegisteredClaims
}

type Manager struct {
	secret          []byte
	accessTokenTTL  time.Duration
	refreshTokenTTL time.Duration
}

func NewManager(secret string, accessTTLMinutes, refreshTTLDays int) *Manager {
	return &Manager{
		secret:          []byte(secret),
		accessTokenTTL:  time.Duration(accessTTLMinutes) * time.Minute,
		refreshTokenTTL: time.Duration(refreshTTLDays) * 24 * time.Hour,
	}
}

func (m *Manager) GenerateAccessToken(userID, role string) (string, error) {
	claims := Claims{
		UserID: userID,
		Role:   role,
		RegisteredClaims: jwtgo.RegisteredClaims{
			ExpiresAt: jwtgo.NewNumericDate(time.Now().Add(m.accessTokenTTL)),
			IssuedAt:  jwtgo.NewNumericDate(time.Now()),
		},
	}

	token := jwtgo.NewWithClaims(jwtgo.SigningMethodHS256, claims)
	return token.SignedString(m.secret)
}

func (m *Manager) GenerateRefreshToken(userID, role string) (string, error) {
	claims := Claims{
		UserID: userID,
		Role:   role,
		RegisteredClaims: jwtgo.RegisteredClaims{
			ExpiresAt: jwtgo.NewNumericDate(time.Now().Add(m.refreshTokenTTL)),
			IssuedAt:  jwtgo.NewNumericDate(time.Now()),
		},
	}

	token := jwtgo.NewWithClaims(jwtgo.SigningMethodHS256, claims)
	return token.SignedString(m.secret)
}

func (m *Manager) Validate(tokenString string) (*Claims, error) {
	token, err := jwtgo.ParseWithClaims(tokenString, &Claims{}, func(t *jwtgo.Token) (interface{}, error) {
		if _, ok := t.Method.(*jwtgo.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("unexpected signing method: %v", t.Header["alg"])
		}
		return m.secret, nil
	})
	if err != nil {
		return nil, fmt.Errorf("parsing token: %w", err)
	}

	claims, ok := token.Claims.(*Claims)
	if !ok || !token.Valid {
		return nil, fmt.Errorf("invalid token claims")
	}

	return claims, nil
}
