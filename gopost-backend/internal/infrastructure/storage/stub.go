package storage

import (
	"context"
	"fmt"
	"io"
	"time"

	"github.com/gopost-app/gopost-backend/internal/repository"
)

// StubStorage satisfies repository.StorageRepository for development when S3/MinIO is not configured.
type StubStorage struct{}

var _ repository.StorageRepository = (*StubStorage)(nil)

func NewStubStorage() *StubStorage {
	return &StubStorage{}
}

func (s *StubStorage) Upload(ctx context.Context, key string, reader io.Reader, contentType string, size int64) error {
	_, _ = io.Copy(io.Discard, reader)
	return nil
}

func (s *StubStorage) Download(ctx context.Context, key string) (io.ReadCloser, error) {
	return nil, fmt.Errorf("stub storage: download not implemented")
}

func (s *StubStorage) Delete(ctx context.Context, key string) error {
	return nil
}

func (s *StubStorage) GenerateSignedURL(ctx context.Context, key string, expiry time.Duration) (string, error) {
	return fmt.Sprintf("https://stub.local/%s?expiry=%d", key, int(expiry.Seconds())), nil
}
