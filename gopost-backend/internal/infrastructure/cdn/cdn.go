package cdn

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"time"
)

type CachePolicy struct {
	TemplateBinaries time.Duration
	Thumbnails       time.Duration
	Previews         time.Duration
}

type CloudflareConfig struct {
	ZoneID      string
	APIToken    string
	CDNBaseURL  string
	CachePolicy CachePolicy
}

func DefaultConfig() CloudflareConfig {
	return CloudflareConfig{
		CDNBaseURL: "https://cdn.gopost.app",
		CachePolicy: CachePolicy{
			TemplateBinaries: 7 * 24 * time.Hour,
			Thumbnails:       30 * 24 * time.Hour,
			Previews:         30 * 24 * time.Hour,
		},
	}
}

type CloudflareCDN struct {
	cfg    CloudflareConfig
	client *http.Client
}

func NewCloudflareCDN(cfg CloudflareConfig) *CloudflareCDN {
	return &CloudflareCDN{
		cfg:    cfg,
		client: &http.Client{Timeout: 15 * time.Second},
	}
}

func (c *CloudflareCDN) TemplateURL(storageKey string) string {
	return c.cfg.CDNBaseURL + "/" + storageKey
}

func (c *CloudflareCDN) ThumbnailURL(storageKey string) string {
	return c.cfg.CDNBaseURL + "/thumbnails/" + storageKey
}

func (c *CloudflareCDN) PreviewURL(storageKey string) string {
	return c.cfg.CDNBaseURL + "/previews/" + storageKey
}

// PurgeByURLs invalidates specific URLs from Cloudflare's edge cache.
func (c *CloudflareCDN) PurgeByURLs(ctx context.Context, urls []string) error {
	if c.cfg.ZoneID == "" || c.cfg.APIToken == "" {
		return nil
	}

	payload := map[string]interface{}{"files": urls}
	body, err := json.Marshal(payload)
	if err != nil {
		return fmt.Errorf("marshaling purge request: %w", err)
	}

	url := fmt.Sprintf("https://api.cloudflare.com/client/v4/zones/%s/purge_cache", c.cfg.ZoneID)
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, url, bytes.NewReader(body))
	if err != nil {
		return fmt.Errorf("creating purge request: %w", err)
	}
	req.Header.Set("Authorization", "Bearer "+c.cfg.APIToken)
	req.Header.Set("Content-Type", "application/json")

	resp, err := c.client.Do(req)
	if err != nil {
		return fmt.Errorf("purging cache: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode >= 400 {
		return fmt.Errorf("cloudflare purge failed with status %d", resp.StatusCode)
	}
	return nil
}

// PurgeByPrefix invalidates all cached content under a URL prefix.
func (c *CloudflareCDN) PurgeByPrefix(ctx context.Context, prefix string) error {
	if c.cfg.ZoneID == "" || c.cfg.APIToken == "" {
		return nil
	}

	payload := map[string]interface{}{"prefixes": []string{prefix}}
	body, err := json.Marshal(payload)
	if err != nil {
		return fmt.Errorf("marshaling purge request: %w", err)
	}

	url := fmt.Sprintf("https://api.cloudflare.com/client/v4/zones/%s/purge_cache", c.cfg.ZoneID)
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, url, bytes.NewReader(body))
	if err != nil {
		return fmt.Errorf("creating purge request: %w", err)
	}
	req.Header.Set("Authorization", "Bearer "+c.cfg.APIToken)
	req.Header.Set("Content-Type", "application/json")

	resp, err := c.client.Do(req)
	if err != nil {
		return fmt.Errorf("purging cache by prefix: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode >= 400 {
		return fmt.Errorf("cloudflare prefix purge failed with status %d", resp.StatusCode)
	}
	return nil
}

// PurgeTemplate invalidates all cached assets for a specific template.
func (c *CloudflareCDN) PurgeTemplate(ctx context.Context, storageKey string) error {
	urls := []string{
		c.TemplateURL(storageKey),
		c.ThumbnailURL(storageKey),
		c.PreviewURL(storageKey),
	}
	return c.PurgeByURLs(ctx, urls)
}

// WarmTemplate sends HEAD requests to template, thumbnail, and preview URLs
// to populate the CDN edge cache. Fire-and-forget; errors are silently ignored.
func (c *CloudflareCDN) WarmTemplate(ctx context.Context, storageKey string) {
	urls := []string{
		c.TemplateURL(storageKey),
		c.ThumbnailURL(storageKey),
		c.PreviewURL(storageKey),
	}
	for _, u := range urls {
		req, err := http.NewRequestWithContext(ctx, http.MethodHead, u, nil)
		if err != nil {
			continue
		}
		resp, err := c.client.Do(req)
		if err != nil {
			continue
		}
		resp.Body.Close()
	}
}
