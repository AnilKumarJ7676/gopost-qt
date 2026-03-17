package service

import (
	"context"
	"fmt"

	jwtgo "github.com/golang-jwt/jwt/v5"
)

type AppleOAuthProvider struct{}

func NewAppleOAuthProvider() OAuthProvider {
	return &AppleOAuthProvider{}
}

func (a *AppleOAuthProvider) ProviderName() string { return "apple" }

func (a *AppleOAuthProvider) ValidateToken(ctx context.Context, idToken string) (*OAuthUserInfo, error) {
	parser := jwtgo.NewParser(jwtgo.WithoutClaimsValidation())
	token, _, err := parser.ParseUnverified(idToken, jwtgo.MapClaims{})
	if err != nil {
		return nil, fmt.Errorf("parsing apple token: %w", err)
	}

	claims, ok := token.Claims.(jwtgo.MapClaims)
	if !ok {
		return nil, fmt.Errorf("invalid apple token claims")
	}

	sub, _ := claims["sub"].(string)
	email, _ := claims["email"].(string)

	if sub == "" || email == "" {
		return nil, fmt.Errorf("missing required apple claims")
	}

	return &OAuthUserInfo{
		Provider: "apple",
		ID:       sub,
		Email:    email,
		Name:     email,
	}, nil
}
