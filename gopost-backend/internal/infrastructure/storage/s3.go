package storage

import (
	"context"
	"fmt"
	"io"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	awsconfig "github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/credentials"
	"github.com/aws/aws-sdk-go-v2/service/s3"
)

type R2Storage struct {
	client   *s3.Client
	bucket   string
	publicURL string
}

type R2Config struct {
	AccountID string
	Bucket    string
	AccessKey string
	SecretKey string
	PublicURL string // Cloudflare R2 public bucket URL or custom domain
}

func (c R2Config) endpoint() string {
	return fmt.Sprintf("https://%s.r2.cloudflarestorage.com", c.AccountID)
}

func NewR2Storage(cfg R2Config) (*R2Storage, error) {
	r2Endpoint := cfg.endpoint()

	awsCfg, err := awsconfig.LoadDefaultConfig(context.Background(),
		awsconfig.WithRegion("auto"),
		awsconfig.WithCredentialsProvider(
			credentials.NewStaticCredentialsProvider(cfg.AccessKey, cfg.SecretKey, ""),
		),
	)
	if err != nil {
		return nil, fmt.Errorf("loading R2 config: %w", err)
	}

	client := s3.NewFromConfig(awsCfg, func(o *s3.Options) {
		o.BaseEndpoint = aws.String(r2Endpoint)
		o.Region = "auto"
	})

	return &R2Storage{client: client, bucket: cfg.Bucket, publicURL: cfg.PublicURL}, nil
}

// NewMinIOStorage creates an S3-compatible client for local development with MinIO.
func NewMinIOStorage(endpoint, bucket, accessKey, secretKey string) (*R2Storage, error) {
	resolver := aws.EndpointResolverWithOptionsFunc(
		func(svc, region string, options ...interface{}) (aws.Endpoint, error) {
			return aws.Endpoint{
				URL:               endpoint,
				HostnameImmutable: true,
			}, nil
		},
	)

	awsCfg, err := awsconfig.LoadDefaultConfig(context.Background(),
		awsconfig.WithRegion("us-east-1"),
		awsconfig.WithEndpointResolverWithOptions(resolver),
		awsconfig.WithCredentialsProvider(
			credentials.NewStaticCredentialsProvider(accessKey, secretKey, ""),
		),
	)
	if err != nil {
		return nil, fmt.Errorf("loading MinIO config: %w", err)
	}

	client := s3.NewFromConfig(awsCfg, func(o *s3.Options) {
		o.UsePathStyle = true
	})

	return &R2Storage{client: client, bucket: bucket, publicURL: endpoint + "/" + bucket}, nil
}

func (r *R2Storage) Upload(ctx context.Context, key string, reader io.Reader, contentType string, size int64) error {
	input := &s3.PutObjectInput{
		Bucket:      aws.String(r.bucket),
		Key:         aws.String(key),
		Body:        reader,
		ContentType: aws.String(contentType),
	}
	if _, err := r.client.PutObject(ctx, input); err != nil {
		return fmt.Errorf("uploading to R2: %w", err)
	}
	return nil
}

func (r *R2Storage) Download(ctx context.Context, key string) (io.ReadCloser, error) {
	output, err := r.client.GetObject(ctx, &s3.GetObjectInput{
		Bucket: aws.String(r.bucket),
		Key:    aws.String(key),
	})
	if err != nil {
		return nil, fmt.Errorf("downloading from R2: %w", err)
	}
	return output.Body, nil
}

func (r *R2Storage) Delete(ctx context.Context, key string) error {
	_, err := r.client.DeleteObject(ctx, &s3.DeleteObjectInput{
		Bucket: aws.String(r.bucket),
		Key:    aws.String(key),
	})
	if err != nil {
		return fmt.Errorf("deleting from R2: %w", err)
	}
	return nil
}

func (r *R2Storage) GenerateSignedURL(ctx context.Context, key string, expiry time.Duration) (string, error) {
	presigner := s3.NewPresignClient(r.client)
	req, err := presigner.PresignGetObject(ctx, &s3.GetObjectInput{
		Bucket: aws.String(r.bucket),
		Key:    aws.String(key),
	}, s3.WithPresignExpires(expiry))
	if err != nil {
		return "", fmt.Errorf("generating signed URL: %w", err)
	}
	return req.URL, nil
}

func (r *R2Storage) PublicURL(key string) string {
	if r.publicURL != "" {
		return r.publicURL + "/" + key
	}
	return key
}
