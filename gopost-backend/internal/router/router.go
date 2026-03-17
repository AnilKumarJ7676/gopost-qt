package router

import (
	"context"
	"crypto/sha256"

	"github.com/gin-gonic/gin"
	"github.com/gopost-app/gopost-backend/internal/config"
	"github.com/gopost-app/gopost-backend/internal/controller"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/cache"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/cdn"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/crypto"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/database"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/search"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/storage"
	"github.com/gopost-app/gopost-backend/internal/repository"
	"github.com/gopost-app/gopost-backend/internal/repository/postgres"
	"github.com/gopost-app/gopost-backend/internal/router/middleware"
	"github.com/gopost-app/gopost-backend/internal/service"
	"github.com/gopost-app/gopost-backend/pkg/jwt"
	"github.com/gopost-app/gopost-backend/pkg/response"
	"github.com/rs/zerolog"
)

func New(log zerolog.Logger, db *database.Postgres, rdb *cache.Redis, cfg *config.Config) *gin.Engine {
	if cfg.Env == "production" {
		gin.SetMode(gin.ReleaseMode)
	}

	r := gin.New()
	r.Use(
		middleware.RequestID(),
		middleware.Logging(log),
		middleware.Recovery(log),
		middleware.CORS(cfg),
	)

	r.GET("/health", func(c *gin.Context) {
		dbOk := db.Ping(c.Request.Context()) == nil
		redisOk := rdb.Ping(c.Request.Context()) == nil
		status := "ok"
		if !dbOk || !redisOk {
			status = "degraded"
		}
		response.OK(c, gin.H{
			"status":   status,
			"services": gin.H{"database": dbOk, "redis": redisOk},
		})
	})

	jwtManager := jwt.NewManager(cfg.JWT.Secret, cfg.JWT.AccessTokenTTL, cfg.JWT.RefreshTokenTTL)

	userRepo := postgres.NewUserRepository(db)
	sessionRepo := postgres.NewSessionRepository(db)
	roleRepo := postgres.NewRoleRepository(db)
	templateRepo := postgres.NewTemplateRepository(db)
	categoryRepo := postgres.NewCategoryRepository(db)
	tagRepo := postgres.NewTagRepository(db)

	authSvc := service.NewAuthService(userRepo, sessionRepo, roleRepo, jwtManager, cfg.JWT.RefreshTokenTTL)
	oauthSvc := service.NewOAuthService(userRepo, sessionRepo, roleRepo, jwtManager, cfg.JWT.RefreshTokenTTL,
		service.NewGoogleOAuthProvider(),
		service.NewAppleOAuthProvider(),
	)

	// --- Elasticsearch (optional) ---
	var searchRepo repository.SearchRepository
	if cfg.ElasticsearchURL != "" {
		es := search.NewElasticsearch(cfg.ElasticsearchURL)
		if err := es.EnsureIndex(context.Background()); err != nil {
			log.Warn().Err(err).Msg("Failed to ensure ES index, search will fall back to Postgres")
		} else {
			searchRepo = es
			log.Info().Str("url", cfg.ElasticsearchURL).Msg("Elasticsearch connected")
		}
	} else {
		log.Info().Msg("Elasticsearch not configured, search uses Postgres ILIKE fallback")
	}

	// --- CDN (optional) ---
	var cdnSvc service.CDN
	if cfg.Cloudflare.ZoneID != "" && cfg.Cloudflare.APIToken != "" {
		cdnInstance := cdn.NewCloudflareCDN(cdn.CloudflareConfig{
			ZoneID:     cfg.Cloudflare.ZoneID,
			APIToken:   cfg.Cloudflare.APIToken,
			CDNBaseURL: cfg.Cloudflare.CDNBaseURL,
		})
		cdnSvc = cdnInstance
		log.Info().Str("cdn_base", cfg.Cloudflare.CDNBaseURL).Msg("Cloudflare CDN configured")
	} else {
		log.Info().Msg("Cloudflare CDN not configured, cache invalidation/warming disabled")
	}

	templateSvc := service.NewTemplateService(templateRepo, categoryRepo, tagRepo, searchRepo, cdnSvc)
	categorySvc := service.NewCategoryService(categoryRepo)

	kekHash := sha256.Sum256([]byte(cfg.JWT.Secret))
	encSvc := crypto.NewEncryptionService(kekHash[:])

	var storageRepo repository.StorageRepository
	if cfg.R2.AccountID != "" {
		r2, err := storage.NewR2Storage(storage.R2Config{
			AccountID: cfg.R2.AccountID,
			Bucket:    cfg.R2.Bucket,
			AccessKey: cfg.R2.AccessKey,
			SecretKey: cfg.R2.SecretKey,
			PublicURL: cfg.R2.PublicURL,
		})
		if err != nil {
			log.Warn().Err(err).Msg("Failed to init R2, falling back to stub storage")
			storageRepo = storage.NewStubStorage()
		} else {
			storageRepo = r2
			log.Info().Msg("Using Cloudflare R2 storage")
		}
	} else {
		storageRepo = storage.NewStubStorage()
		log.Info().Msg("R2 not configured, using stub storage")
	}

	authCtrl := controller.NewAuthController(authSvc, oauthSvc)
	templateCtrl := controller.NewTemplateController(templateSvc, categorySvc)
	accessCtrl := controller.NewTemplateAccessController(templateSvc, storageRepo, encSvc, cdnSvc)
	adminCtrl := controller.NewAdminController(templateSvc, storageRepo, encSvc)

	v1 := r.Group("/api/v1")

	auth := v1.Group("/auth")
	auth.Use(middleware.RateLimit(rdb, "auth"))
	{
		auth.POST("/register", authCtrl.Register)
		auth.POST("/login", authCtrl.Login)
		auth.POST("/refresh", authCtrl.RefreshToken)
		auth.POST("/logout", authCtrl.Logout)
		auth.POST("/oauth/:provider", authCtrl.OAuthLogin)
	}

	templates := v1.Group("/templates")
	templates.Use(middleware.RateLimit(rdb, "browsing"))
	{
		templates.GET("", templateCtrl.List)
		templates.GET("/search", templateCtrl.Search)
		templates.GET("/:id", templateCtrl.GetByID)
	}

	categories := v1.Group("/categories")
	{
		categories.GET("", templateCtrl.ListCategories)
		categories.GET("/:id/templates", templateCtrl.ListByCategory)
	}

	protected := v1.Group("")
	protected.Use(middleware.Auth(jwtManager))
	protected.Use(middleware.RateLimit(rdb, "browsing"))
	{
		protected.GET("/me", func(c *gin.Context) {
			response.OK(c, gin.H{"user_id": c.GetString("user_id"), "role": c.GetString("user_role")})
		})
		protected.POST("/templates/:id/access", accessCtrl.RequestAccess)
	}

	admin := v1.Group("/admin")
	admin.Use(middleware.Auth(jwtManager))
	admin.Use(middleware.RequireRole("admin"))
	{
		admin.GET("/health", func(c *gin.Context) {
			response.OK(c, gin.H{"admin": true})
		})
		admin.POST("/templates/upload", adminCtrl.UploadTemplate)
		admin.POST("/templates/create-from-editor", adminCtrl.CreateFromEditor)
	}

	return r
}
