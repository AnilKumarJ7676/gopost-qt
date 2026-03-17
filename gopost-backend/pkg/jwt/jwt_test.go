package jwt

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestGenerateAndValidateAccessToken(t *testing.T) {
	m := NewManager("test-secret", 15, 7)

	token, err := m.GenerateAccessToken("user-123", "user")
	require.NoError(t, err)
	assert.NotEmpty(t, token)

	claims, err := m.Validate(token)
	require.NoError(t, err)
	assert.Equal(t, "user-123", claims.UserID)
	assert.Equal(t, "user", claims.Role)
}

func TestExpiredToken(t *testing.T) {
	m := &Manager{
		secret:         []byte("test-secret"),
		accessTokenTTL: -1 * time.Hour,
	}

	token, err := m.GenerateAccessToken("user-123", "user")
	require.NoError(t, err)

	_, err = m.Validate(token)
	assert.Error(t, err)
}

func TestTamperedToken(t *testing.T) {
	m := NewManager("test-secret", 15, 7)

	token, err := m.GenerateAccessToken("user-123", "user")
	require.NoError(t, err)

	tampered := token + "x"
	_, err = m.Validate(tampered)
	assert.Error(t, err)
}

func TestWrongSecret(t *testing.T) {
	m1 := NewManager("secret-one", 15, 7)
	m2 := NewManager("secret-two", 15, 7)

	token, err := m1.GenerateAccessToken("user-123", "user")
	require.NoError(t, err)

	_, err = m2.Validate(token)
	assert.Error(t, err)
}
