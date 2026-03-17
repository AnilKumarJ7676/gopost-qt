package search

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/gopost-app/gopost-backend/internal/domain/entity"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/google/uuid"
)

type Elasticsearch struct {
	baseURL string
	index   string
	client  *http.Client
}

func NewElasticsearch(baseURL string) *Elasticsearch {
	return &Elasticsearch{
		baseURL: baseURL,
		index:   "templates",
		client:  &http.Client{Timeout: 10 * time.Second},
	}
}

// EnsureIndex creates the templates index with the correct mapping if it
// does not already exist. Safe to call on every startup.
func (es *Elasticsearch) EnsureIndex(ctx context.Context) error {
	url := fmt.Sprintf("%s/%s", es.baseURL, es.index)
	req, err := http.NewRequestWithContext(ctx, http.MethodHead, url, nil)
	if err != nil {
		return fmt.Errorf("checking index: %w", err)
	}
	resp, err := es.client.Do(req)
	if err != nil {
		return fmt.Errorf("checking index: %w", err)
	}
	resp.Body.Close()
	if resp.StatusCode == 200 {
		return nil
	}

	mapping := map[string]interface{}{
		"mappings": map[string]interface{}{
			"properties": map[string]interface{}{
				"name":        map[string]string{"type": "text", "analyzer": "standard"},
				"description": map[string]string{"type": "text", "analyzer": "standard"},
				"tags":        map[string]string{"type": "text", "analyzer": "standard"},
				"type":        map[string]string{"type": "keyword"},
				"status":      map[string]string{"type": "keyword"},
				"category_id": map[string]string{"type": "keyword"},
				"is_premium":  map[string]string{"type": "boolean"},
				"usage_count": map[string]string{"type": "integer"},
				"created_at":  map[string]string{"type": "date"},
			},
		},
	}
	body, _ := json.Marshal(mapping)
	createReq, err := http.NewRequestWithContext(ctx, http.MethodPut, url, bytes.NewReader(body))
	if err != nil {
		return fmt.Errorf("creating index: %w", err)
	}
	createReq.Header.Set("Content-Type", "application/json")
	createResp, err := es.client.Do(createReq)
	if err != nil {
		return fmt.Errorf("creating index: %w", err)
	}
	defer createResp.Body.Close()
	if createResp.StatusCode >= 400 {
		return fmt.Errorf("failed to create ES index, status %d", createResp.StatusCode)
	}
	return nil
}

type esTemplateDoc struct {
	ID          string   `json:"id"`
	Name        string   `json:"name"`
	Description string   `json:"description"`
	Type        string   `json:"type"`
	CategoryID  string   `json:"category_id,omitempty"`
	Status      string   `json:"status"`
	IsPremium   bool     `json:"is_premium"`
	UsageCount  int      `json:"usage_count"`
	Tags        []string `json:"tags,omitempty"`
	CreatedAt   string   `json:"created_at"`
}

func (es *Elasticsearch) IndexTemplate(ctx context.Context, t *entity.Template) error {
	doc := esTemplateDoc{
		ID:          t.ID.String(),
		Name:        t.Name,
		Description: t.Description,
		Type:        string(t.Type),
		Status:      string(t.Status),
		IsPremium:   t.IsPremium,
		UsageCount:  t.UsageCount,
		CreatedAt:   t.CreatedAt.Format(time.RFC3339),
	}
	if t.CategoryID != nil {
		doc.CategoryID = t.CategoryID.String()
	}
	for _, tag := range t.Tags {
		doc.Tags = append(doc.Tags, tag.Name)
	}

	body, _ := json.Marshal(doc)
	url := fmt.Sprintf("%s/%s/_doc/%s", es.baseURL, es.index, t.ID.String())
	req, err := http.NewRequestWithContext(ctx, http.MethodPut, url, bytes.NewReader(body))
	if err != nil {
		return fmt.Errorf("creating ES request: %w", err)
	}
	req.Header.Set("Content-Type", "application/json")

	resp, err := es.client.Do(req)
	if err != nil {
		return fmt.Errorf("indexing template: %w", err)
	}
	defer resp.Body.Close()
	return nil
}

func (es *Elasticsearch) DeleteTemplate(ctx context.Context, id uuid.UUID) error {
	url := fmt.Sprintf("%s/%s/_doc/%s", es.baseURL, es.index, id.String())
	req, err := http.NewRequestWithContext(ctx, http.MethodDelete, url, nil)
	if err != nil {
		return fmt.Errorf("creating delete request: %w", err)
	}
	resp, err := es.client.Do(req)
	if err != nil {
		return fmt.Errorf("deleting from ES: %w", err)
	}
	defer resp.Body.Close()
	return nil
}

func (es *Elasticsearch) SearchTemplates(ctx context.Context, query string, filter repository.TemplateFilter, page repository.CursorPagination) (*repository.PaginatedResult[entity.Template], error) {
	limit := page.Limit
	if limit <= 0 {
		limit = 20
	}

	must := []map[string]interface{}{
		{"multi_match": map[string]interface{}{
			"query":  query,
			"fields": []string{"name^3", "description", "tags^2"},
		}},
	}

	filterClauses := []map[string]interface{}{}
	if filter.Type != "" {
		filterClauses = append(filterClauses, map[string]interface{}{"term": map[string]string{"type": filter.Type}})
	}
	if filter.Status != "" {
		filterClauses = append(filterClauses, map[string]interface{}{"term": map[string]string{"status": filter.Status}})
	}
	if filter.IsPremium != nil {
		filterClauses = append(filterClauses, map[string]interface{}{"term": map[string]bool{"is_premium": *filter.IsPremium}})
	}

	esQuery := map[string]interface{}{
		"size": limit,
		"query": map[string]interface{}{
			"bool": map[string]interface{}{
				"must":   must,
				"filter": filterClauses,
			},
		},
	}

	body, _ := json.Marshal(esQuery)
	url := fmt.Sprintf("%s/%s/_search", es.baseURL, es.index)
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, url, bytes.NewReader(body))
	if err != nil {
		return nil, fmt.Errorf("creating search request: %w", err)
	}
	req.Header.Set("Content-Type", "application/json")

	resp, err := es.client.Do(req)
	if err != nil {
		return nil, fmt.Errorf("searching ES: %w", err)
	}
	defer resp.Body.Close()

	var result struct {
		Hits struct {
			Total struct{ Value int64 } `json:"total"`
			Hits  []struct {
				Source esTemplateDoc `json:"_source"`
			} `json:"hits"`
		} `json:"hits"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&result); err != nil {
		return nil, fmt.Errorf("decoding ES response: %w", err)
	}

	var items []entity.Template
	for _, hit := range result.Hits.Hits {
		id, _ := uuid.Parse(hit.Source.ID)
		t := entity.Template{
			ID:          id,
			Name:        hit.Source.Name,
			Description: hit.Source.Description,
			Type:        entity.TemplateType(hit.Source.Type),
			Status:      entity.TemplateStatus(hit.Source.Status),
			IsPremium:   hit.Source.IsPremium,
			UsageCount:  hit.Source.UsageCount,
		}
		items = append(items, t)
	}

	return &repository.PaginatedResult[entity.Template]{
		Items: items, Total: result.Hits.Total.Value, HasMore: int64(len(items)) < result.Hits.Total.Value,
	}, nil
}
