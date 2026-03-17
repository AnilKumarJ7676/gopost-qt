package controller

import (
	"fmt"
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/gopost-app/gopost-backend/internal/service"
	"github.com/gopost-app/gopost-backend/pkg/response"
	"github.com/google/uuid"
)

type TemplateController struct {
	templateSvc service.TemplateService
	categorySvc service.CategoryService
}

func NewTemplateController(templateSvc service.TemplateService, categorySvc service.CategoryService) *TemplateController {
	return &TemplateController{templateSvc: templateSvc, categorySvc: categorySvc}
}

func (ctrl *TemplateController) List(c *gin.Context) {
	filter := repository.TemplateFilter{
		Type:   c.Query("type"),
		Status: c.DefaultQuery("status", "published"),
		Sort:   c.DefaultQuery("sort", "newest"),
		Query:  c.Query("q"),
	}
	if catID := c.Query("category_id"); catID != "" {
		id, err := uuid.Parse(catID)
		if err == nil {
			filter.CategoryID = &id
		}
	}
	if c.Query("is_premium") == "true" {
		v := true
		filter.IsPremium = &v
	} else if c.Query("is_premium") == "false" {
		v := false
		filter.IsPremium = &v
	}

	page := parsePagination(c)
	result, err := ctrl.templateSvc.List(c.Request.Context(), filter, page)
	if err != nil {
		handleServiceError(c, err)
		return
	}
	response.OK(c, result)
}

func (ctrl *TemplateController) GetByID(c *gin.Context) {
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
	response.OK(c, t)
}

func (ctrl *TemplateController) Search(c *gin.Context) {
	query := c.Query("q")
	if query == "" {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "Search query is required")
		return
	}
	filter := repository.TemplateFilter{
		Status: "published",
		Type:   c.Query("type"),
		Sort:   c.DefaultQuery("sort", "popular"),
	}
	if c.Query("is_premium") == "true" {
		v := true
		filter.IsPremium = &v
	} else if c.Query("is_premium") == "false" {
		v := false
		filter.IsPremium = &v
	}
	page := parsePagination(c)
	result, err := ctrl.templateSvc.Search(c.Request.Context(), query, filter, page)
	if err != nil {
		handleServiceError(c, err)
		return
	}
	response.OK(c, result)
}

func (ctrl *TemplateController) ListCategories(c *gin.Context) {
	cats, err := ctrl.categorySvc.List(c.Request.Context())
	if err != nil {
		handleServiceError(c, err)
		return
	}
	response.OK(c, cats)
}

func (ctrl *TemplateController) ListByCategory(c *gin.Context) {
	id, err := uuid.Parse(c.Param("id"))
	if err != nil {
		response.Error(c, http.StatusBadRequest, "INVALID_ID", "Invalid category ID")
		return
	}
	page := parsePagination(c)
	result, err := ctrl.templateSvc.ListByCategory(c.Request.Context(), id, page)
	if err != nil {
		handleServiceError(c, err)
		return
	}
	response.OK(c, result)
}

func parsePagination(c *gin.Context) repository.CursorPagination {
	limit := 20
	if l := c.Query("limit"); l != "" {
		if v, err := parseIntParam(l); err == nil && v > 0 && v <= 50 {
			limit = v
		}
	}
	return repository.CursorPagination{
		Cursor: c.Query("cursor"),
		Limit:  limit,
	}
}

func parseIntParam(s string) (int, error) {
	var v int
	_, err := fmt.Sscanf(s, "%d", &v)
	return v, err
}
