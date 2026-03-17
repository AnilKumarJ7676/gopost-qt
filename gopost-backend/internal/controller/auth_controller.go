package controller

import (
	"errors"
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/internal/service"
	"github.com/gopost-app/gopost-backend/pkg/response"
	"github.com/gopost-app/gopost-backend/pkg/validator"
)

type AuthController struct {
	authService  service.AuthService
	oauthService service.OAuthService
}

func NewAuthController(authService service.AuthService, oauthService service.OAuthService) *AuthController {
	return &AuthController{
		authService:  authService,
		oauthService: oauthService,
	}
}

func (ctrl *AuthController) Register(c *gin.Context) {
	var req service.RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "Invalid request body")
		return
	}

	var errs []response.FieldError
	if e := validator.ValidateRequired("name", req.Name); e != nil {
		errs = append(errs, *e)
	}
	if e := validator.ValidateEmail(req.Email); e != nil {
		errs = append(errs, *e)
	}
	if e := validator.ValidatePassword(req.Password); e != nil {
		errs = append(errs, *e)
	}
	if len(errs) > 0 {
		response.ValidationError(c, errs)
		return
	}

	res, err := ctrl.authService.Register(c.Request.Context(), req)
	if err != nil {
		handleServiceError(c, err)
		return
	}

	response.Created(c, res)
}

func (ctrl *AuthController) Login(c *gin.Context) {
	var req service.LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "Invalid request body")
		return
	}

	var errs []response.FieldError
	if e := validator.ValidateEmail(req.Email); e != nil {
		errs = append(errs, *e)
	}
	if e := validator.ValidatePassword(req.Password); e != nil {
		errs = append(errs, *e)
	}
	if len(errs) > 0 {
		response.ValidationError(c, errs)
		return
	}

	res, err := ctrl.authService.Login(c.Request.Context(), req)
	if err != nil {
		handleServiceError(c, err)
		return
	}

	response.OK(c, res)
}

func (ctrl *AuthController) RefreshToken(c *gin.Context) {
	var req struct {
		RefreshToken string `json:"refresh_token"`
	}
	if err := c.ShouldBindJSON(&req); err != nil || req.RefreshToken == "" {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "refresh_token is required")
		return
	}

	res, err := ctrl.authService.RefreshToken(c.Request.Context(), req.RefreshToken, c.Request)
	if err != nil {
		handleServiceError(c, err)
		return
	}

	response.OK(c, res)
}

func (ctrl *AuthController) Logout(c *gin.Context) {
	var req struct {
		RefreshToken string `json:"refresh_token"`
	}
	if err := c.ShouldBindJSON(&req); err != nil || req.RefreshToken == "" {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "refresh_token is required")
		return
	}

	if err := ctrl.authService.Logout(c.Request.Context(), req.RefreshToken); err != nil {
		handleServiceError(c, err)
		return
	}

	response.OK(c, gin.H{"message": "Logged out successfully"})
}

func (ctrl *AuthController) OAuthLogin(c *gin.Context) {
	provider := c.Param("provider")
	var req struct {
		IDToken string `json:"id_token"`
	}
	if err := c.ShouldBindJSON(&req); err != nil || req.IDToken == "" {
		response.Error(c, http.StatusBadRequest, "INVALID_INPUT", "id_token is required")
		return
	}

	res, err := ctrl.oauthService.Authenticate(c.Request.Context(), provider, req.IDToken)
	if err != nil {
		handleServiceError(c, err)
		return
	}

	response.OK(c, res)
}

func handleServiceError(c *gin.Context, err error) {
	var appErr *service.AppError
	if errors.As(err, &appErr) {
		response.Error(c, appErr.Status, appErr.Code, appErr.Message)
		return
	}
	response.Error(c, http.StatusInternalServerError, "INTERNAL_ERROR", "An internal error occurred")
}
