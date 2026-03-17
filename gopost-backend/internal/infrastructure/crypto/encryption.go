package crypto

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"encoding/base64"
	"encoding/binary"
	"fmt"
	"io"
)

var gptMagic = []byte("GOPT")

const gptVersion uint16 = 1

type EncryptionService struct {
	kek []byte // Key Encryption Key
}

func NewEncryptionService(kek []byte) *EncryptionService {
	return &EncryptionService{kek: kek}
}

func (e *EncryptionService) Encrypt(plaintext []byte) ([]byte, []byte, error) {
	dek := make([]byte, 32)
	if _, err := io.ReadFull(rand.Reader, dek); err != nil {
		return nil, nil, fmt.Errorf("generating DEK: %w", err)
	}

	block, err := aes.NewCipher(dek)
	if err != nil {
		return nil, nil, fmt.Errorf("creating cipher: %w", err)
	}

	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, nil, fmt.Errorf("creating GCM: %w", err)
	}

	nonce := make([]byte, gcm.NonceSize())
	if _, err := io.ReadFull(rand.Reader, nonce); err != nil {
		return nil, nil, fmt.Errorf("generating nonce: %w", err)
	}

	ciphertext := gcm.Seal(nonce, nonce, plaintext, nil)

	wrappedDEK, err := e.wrapKey(dek)
	if err != nil {
		return nil, nil, fmt.Errorf("wrapping DEK: %w", err)
	}

	return ciphertext, wrappedDEK, nil
}

func (e *EncryptionService) Decrypt(ciphertext, wrappedDEK []byte) ([]byte, error) {
	dek, err := e.unwrapKey(wrappedDEK)
	if err != nil {
		return nil, fmt.Errorf("unwrapping DEK: %w", err)
	}

	block, err := aes.NewCipher(dek)
	if err != nil {
		return nil, fmt.Errorf("creating cipher: %w", err)
	}

	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, fmt.Errorf("creating GCM: %w", err)
	}

	nonceSize := gcm.NonceSize()
	if len(ciphertext) < nonceSize {
		return nil, fmt.Errorf("ciphertext too short")
	}

	nonce, ciphertext := ciphertext[:nonceSize], ciphertext[nonceSize:]
	plaintext, err := gcm.Open(nil, nonce, ciphertext, nil)
	if err != nil {
		return nil, fmt.Errorf("decrypting: %w", err)
	}

	return plaintext, nil
}

// SessionKeyForTemplate returns a base64-encoded session key for the client to decrypt
// the template. When templates are stored encrypted with a per-template DEK, look up
// the wrapped DEK (e.g. by storageKey) and unwrap here; for now returns a placeholder.
func (e *EncryptionService) SessionKeyForTemplate(_ string) string {
	placeholder := make([]byte, 32)
	return base64.StdEncoding.EncodeToString(placeholder)
}

func (e *EncryptionService) PackGPT(metadata, encryptedPayload, wrappedDEK []byte) ([]byte, error) {
	var buf bytes.Buffer

	buf.Write(gptMagic)
	_ = binary.Write(&buf, binary.BigEndian, gptVersion)
	_ = binary.Write(&buf, binary.BigEndian, uint32(len(wrappedDEK)))
	buf.Write(wrappedDEK)
	_ = binary.Write(&buf, binary.BigEndian, uint32(len(metadata)))
	buf.Write(metadata)
	_ = binary.Write(&buf, binary.BigEndian, uint32(len(encryptedPayload)))
	buf.Write(encryptedPayload)

	return buf.Bytes(), nil
}

func (e *EncryptionService) UnpackGPT(data []byte) (metadata, encryptedPayload, wrappedDEK []byte, err error) {
	r := bytes.NewReader(data)

	magic := make([]byte, 4)
	if _, err := io.ReadFull(r, magic); err != nil || !bytes.Equal(magic, gptMagic) {
		return nil, nil, nil, fmt.Errorf("invalid .gpt magic bytes")
	}

	var version uint16
	if err := binary.Read(r, binary.BigEndian, &version); err != nil {
		return nil, nil, nil, fmt.Errorf("reading version: %w", err)
	}

	var dekLen uint32
	_ = binary.Read(r, binary.BigEndian, &dekLen)
	wrappedDEK = make([]byte, dekLen)
	_, _ = io.ReadFull(r, wrappedDEK)

	var metaLen uint32
	_ = binary.Read(r, binary.BigEndian, &metaLen)
	metadata = make([]byte, metaLen)
	_, _ = io.ReadFull(r, metadata)

	var payloadLen uint32
	_ = binary.Read(r, binary.BigEndian, &payloadLen)
	encryptedPayload = make([]byte, payloadLen)
	_, _ = io.ReadFull(r, encryptedPayload)

	return metadata, encryptedPayload, wrappedDEK, nil
}

func (e *EncryptionService) wrapKey(dek []byte) ([]byte, error) {
	block, err := aes.NewCipher(e.kek)
	if err != nil {
		return nil, err
	}
	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, err
	}
	nonce := make([]byte, gcm.NonceSize())
	if _, err := io.ReadFull(rand.Reader, nonce); err != nil {
		return nil, err
	}
	return gcm.Seal(nonce, nonce, dek, nil), nil
}

func (e *EncryptionService) unwrapKey(wrapped []byte) ([]byte, error) {
	block, err := aes.NewCipher(e.kek)
	if err != nil {
		return nil, err
	}
	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, err
	}
	nonceSize := gcm.NonceSize()
	if len(wrapped) < nonceSize {
		return nil, fmt.Errorf("wrapped key too short")
	}
	nonce, ciphertext := wrapped[:nonceSize], wrapped[nonceSize:]
	return gcm.Open(nil, nonce, ciphertext, nil)
}
