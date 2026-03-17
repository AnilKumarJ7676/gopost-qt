package controller

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/crypto"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/gopost-app/gopost-backend/internal/service"
	"github.com/gopost-app/gopost-backend/pkg/response"
	"github.com/google/uuid"
)

type AdminController struct {
	templateSvc   service.TemplateService
	storageRepo   repository.StorageRepository
	encryptionSvc *crypto.EncryptionService
}

func NewAdminController(
	templateSvc service.TemplateService,
	storageRepo repository.StorageRepository,
	encryptionSvc *crypto.EncryptionService,
) *AdminController {
	return &AdminController{
		templateSvc:   templateSvc,
		storageRepo:   storageRepo,
		encryptionSvc: encryptionSvc,
	}
}

func (ctrl *AdminController) UploadTemplate(c *gin.Context) {
	file, header, err := c.Request.FormFile("file")
	if err != nil {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "File is required")
		return
	}
	defer file.Close()

	maxSize := int64(100 * 1024 * 1024) // 100MB
	if header.Size > maxSize {
		response.Error(c, http.StatusBadRequest, "FILE_TOO_LARGE", "File exceeds 100MB limit")
		return
	}

	name := c.PostForm("name")
	if name == "" {
		name = header.Filename
	}

	templateType := c.DefaultPostForm("type", "image")
	if templateType != "video" && templateType != "image" {
		response.Error(c, http.StatusBadRequest, "INVALID_TYPE", "Type must be 'video' or 'image'")
		return
	}

	storageKey := fmt.Sprintf("templates/%s/%s", uuid.New().String(), header.Filename)

	if err := ctrl.storageRepo.Upload(c.Request.Context(), storageKey, file, header.Header.Get("Content-Type"), header.Size); err != nil {
		response.Error(c, http.StatusInternalServerError, "UPLOAD_FAILED", "Failed to upload file")
		return
	}

	var categoryID *uuid.UUID
	if catStr := c.PostForm("category_id"); catStr != "" {
		id, err := uuid.Parse(catStr)
		if err == nil {
			categoryID = &id
		}
	}

	creatorIDStr := c.GetString("user_id")
	var creatorID *uuid.UUID
	if id, err := uuid.Parse(creatorIDStr); err == nil {
		creatorID = &id
	}

	t, err := ctrl.templateSvc.Create(c.Request.Context(), service.CreateTemplateRequest{
		Name:       name,
		Type:       templateType,
		CategoryID: categoryID,
		CreatorID:  creatorID,
		StorageKey: storageKey,
	})
	if err != nil {
		handleServiceError(c, err)
		return
	}

	response.Created(c, t)
}

// CreateFromEditorRequest is the JSON body for creating a template from editor content.
type CreateFromEditorRequest struct {
	Name           string                 `json:"name"`
	Description    string                 `json:"description,omitempty"`
	Type           string                 `json:"type"`
	CategoryID     string                 `json:"category_id,omitempty"`
	Tags           []string               `json:"tags,omitempty"`
	Width          int                    `json:"width"`
	Height         int                    `json:"height"`
	LayerCount     int                    `json:"layer_count"`
	EditableFields []EditableFieldPayload `json:"editable_fields,omitempty"`
	ContentBase64  string                 `json:"content_base64"`
}

type EditableFieldPayload struct {
	Key          string `json:"key"`
	Label        string `json:"label"`
	FieldType    string `json:"field_type"`
	DefaultValue string `json:"default_value,omitempty"`
}

func (ctrl *AdminController) CreateFromEditor(c *gin.Context) {
	var req CreateFromEditorRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "Invalid JSON body")
		return
	}
	if req.Name == "" {
		response.Error(c, http.StatusBadRequest, "VALIDATION_ERROR", "name is required")
		return
	}
	if req.Type != "video" && req.Type != "image" {
		req.Type = "image"
	}
	if req.Width <= 0 {
		req.Width = 1080
	}
	if req.Height <= 0 {
		req.Height = 1080
	}

	payload, err := base64.StdEncoding.DecodeString(req.ContentBase64)
	if err != nil {
		response.Error(c, http.StatusBadRequest, "INVALID_CONTENT", "content_base64 must be valid base64")
		return
	}
	if len(payload) > 50*1024*1024 {
		response.Error(c, http.StatusBadRequest, "CONTENT_TOO_LARGE", "Content exceeds 50MB limit")
		return
	}

	ciphertext, wrappedDEK, err := ctrl.encryptionSvc.Encrypt(payload)
	if err != nil {
		response.Error(c, http.StatusInternalServerError, "ENCRYPT_FAILED", "Failed to encrypt content")
		return
	}

	meta := map[string]interface{}{
		"name":            req.Name,
		"description":     req.Description,
		"type":            req.Type,
		"width":           req.Width,
		"height":          req.Height,
		"layer_count":     req.LayerCount,
		"editable_fields": req.EditableFields,
	}
	metaBytes, _ := json.Marshal(meta)

	gptBytes, err := ctrl.encryptionSvc.PackGPT(metaBytes, ciphertext, wrappedDEK)
	if err != nil {
		response.Error(c, http.StatusInternalServerError, "PACK_FAILED", "Failed to pack .gpt")
		return
	}

	storageKey := fmt.Sprintf("templates/%s/%s.gpt", uuid.New().String(), req.Name)
	if err := ctrl.storageRepo.Upload(c.Request.Context(), storageKey, bytes.NewReader(gptBytes), "application/octet-stream", int64(len(gptBytes))); err != nil {
		response.Error(c, http.StatusInternalServerError, "UPLOAD_FAILED", "Failed to upload template")
		return
	}

	creatorIDStr := c.GetString("user_id")
	var creatorID *uuid.UUID
	if id, err := uuid.Parse(creatorIDStr); err == nil {
		creatorID = &id
	}

	var categoryID *uuid.UUID
	if req.CategoryID != "" {
		if id, err := uuid.Parse(req.CategoryID); err == nil {
			categoryID = &id
		}
	}

	t, err := ctrl.templateSvc.Create(c.Request.Context(), service.CreateTemplateRequest{
		Name:           req.Name,
		Description:    req.Description,
		Type:           req.Type,
		StorageKey:     storageKey,
		CategoryID:     categoryID,
		CreatorID:      creatorID,
		Tags:           req.Tags,
		Width:          req.Width,
		Height:         req.Height,
		LayerCount:     req.LayerCount,
		EditableFields: req.EditableFields,
	})
	if err != nil {
		handleServiceError(c, err)
		return
	}

	response.Created(c, t)
}

type TemplateAccessController struct {
	templateSvc   service.TemplateService
	storageRepo   repository.StorageRepository
	encryptionSvc *crypto.EncryptionService
	cdn           service.CDN
}

func NewTemplateAccessController(
	templateSvc service.TemplateService,
	storageRepo repository.StorageRepository,
	encryptionSvc *crypto.EncryptionService,
	cdn service.CDN,
) *TemplateAccessController {
	return &TemplateAccessController{
		templateSvc:   templateSvc,
		storageRepo:   storageRepo,
		encryptionSvc: encryptionSvc,
		cdn:           cdn,
	}
}

func (ctrl *TemplateAccessController) RequestAccess(c *gin.Context) {
	id, err := uuid.Parse(c.Param("id"))
	if err != nil {
		response.Error(c, http.StatusBadRequest, "INVALID_ID", "Invalid template ID")
		return
	}

	t, err := ctrl.templateSvc.GetByID(c.Request.Context(), id)
	if err != nil {
		handleServiceError(c, err)
		return
	}

	if t.StorageKey == "" {
		response.Error(c, http.StatusNotFound, "NO_CONTENT", "Template has no downloadable content")
		return
	}

	expiry := 10 * time.Minute

	var downloadURL string
	if ctrl.cdn != nil {
		downloadURL = ctrl.cdn.TemplateURL(t.StorageKey)
	} else {
		var err error
		downloadURL, err = ctrl.storageRepo.GenerateSignedURL(c.Request.Context(), t.StorageKey, expiry)
		if err != nil {
			response.Error(c, http.StatusInternalServerError, "SIGNED_URL_FAILED", "Failed to generate access URL")
			return
		}
	}

	_ = ctrl.templateSvc.IncrementUsage(c.Request.Context(), id)

	expiresAt := time.Now().Add(expiry).UTC().Format(time.RFC3339)
	// session_key: base64 DEK for client to decrypt template. When templates are stored
	// encrypted, persist wrapped DEK per template and unwrap here; for now return placeholder.
	sessionKey := ctrl.encryptionSvc.SessionKeyForTemplate(t.StorageKey)
	renderToken := id.String() + "-" + time.Now().Add(expiry).UTC().Format("20060102150405")

	response.OK(c, gin.H{
		"signed_url":    downloadURL,
		"session_key":   sessionKey,
		"render_token":  renderToken,
		"expires_at":    expiresAt,
		"expires_in":    600,
		"template_id":   t.ID,
		"template_name": t.Name,
		"template_type": t.Type,
	})
}
