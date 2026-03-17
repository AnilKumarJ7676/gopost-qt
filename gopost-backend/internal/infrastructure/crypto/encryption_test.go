package crypto

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestEncryptDecrypt(t *testing.T) {
	kek := make([]byte, 32)
	for i := range kek {
		kek[i] = byte(i)
	}
	svc := NewEncryptionService(kek)

	plaintext := []byte("Hello, Gopost templates!")
	ciphertext, wrappedDEK, err := svc.Encrypt(plaintext)
	require.NoError(t, err)
	assert.NotEqual(t, plaintext, ciphertext)

	decrypted, err := svc.Decrypt(ciphertext, wrappedDEK)
	require.NoError(t, err)
	assert.Equal(t, plaintext, decrypted)
}

func TestPackUnpackGPT(t *testing.T) {
	kek := make([]byte, 32)
	for i := range kek {
		kek[i] = byte(i)
	}
	svc := NewEncryptionService(kek)

	metadata := []byte(`{"name":"test","type":"video"}`)
	payload := []byte("encrypted-payload-data")
	wrappedDEK := []byte("wrapped-dek-bytes")

	packed, err := svc.PackGPT(metadata, payload, wrappedDEK)
	require.NoError(t, err)

	gotMeta, gotPayload, gotDEK, err := svc.UnpackGPT(packed)
	require.NoError(t, err)
	assert.Equal(t, metadata, gotMeta)
	assert.Equal(t, payload, gotPayload)
	assert.Equal(t, wrappedDEK, gotDEK)
}

func TestFullGPTFlow(t *testing.T) {
	kek := make([]byte, 32)
	for i := range kek {
		kek[i] = byte(i)
	}
	svc := NewEncryptionService(kek)

	originalPayload := []byte("template layer data with render instructions")
	metadata := []byte(`{"name":"Summer Vibes","type":"video","version":1}`)

	encrypted, wrappedDEK, err := svc.Encrypt(originalPayload)
	require.NoError(t, err)

	packed, err := svc.PackGPT(metadata, encrypted, wrappedDEK)
	require.NoError(t, err)

	gotMeta, gotEncrypted, gotDEK, err := svc.UnpackGPT(packed)
	require.NoError(t, err)
	assert.Equal(t, metadata, gotMeta)

	decrypted, err := svc.Decrypt(gotEncrypted, gotDEK)
	require.NoError(t, err)
	assert.Equal(t, originalPayload, decrypted)
}
