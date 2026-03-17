package service

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
)

type GoogleOAuthProvider struct {
	httpClient *http.Client
}

func NewGoogleOAuthProvider() OAuthProvider {
	return &GoogleOAuthProvider{httpClient: &http.Client{}}
}

func (g *GoogleOAuthProvider) ProviderName() string { return "google" }

func (g *GoogleOAuthProvider) ValidateToken(ctx context.Context, idToken string) (*OAuthUserInfo, error) {
	url := fmt.Sprintf("https://oauth2.googleapis.com/tokeninfo?id_token=%s", idToken)
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, url, nil)
	if err != nil {
		return nil, err
	}

	resp, err := g.httpClient.Do(req)
	if err != nil {
		return nil, fmt.Errorf("validating google token: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("google token validation failed: status %d", resp.StatusCode)
	}

	var claims struct {
		Sub     string `json:"sub"`
		Email   string `json:"email"`
		Name    string `json:"name"`
		Picture string `json:"picture"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&claims); err != nil {
		return nil, fmt.Errorf("decoding google response: %w", err)
	}

	return &OAuthUserInfo{
		Provider: "google",
		ID:       claims.Sub,
		Email:    claims.Email,
		Name:     claims.Name,
		Avatar:   claims.Picture,
	}, nil
}
