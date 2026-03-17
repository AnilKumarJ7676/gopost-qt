package main

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gopost-app/gopost-backend/internal/config"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/cache"
	"github.com/gopost-app/gopost-backend/internal/infrastructure/database"
	"github.com/gopost-app/gopost-backend/internal/router"
	"github.com/gopost-app/gopost-backend/pkg/logger"
)

func main() {
	cfg, err := config.Load()
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to load config: %v\n", err)
		os.Exit(1)
	}

	log := logger.New(cfg.LogLevel)

	if cfg.MigrationsPath != "" {
		log.Info().Str("path", cfg.MigrationsPath).Msg("running database migrations")
		if err := database.RunMigrations(cfg.Database.DSN(), cfg.MigrationsPath); err != nil {
			log.Fatal().Err(err).Msg("failed to run migrations")
		}
		log.Info().Msg("migrations complete")
	}

	db, err := database.NewPostgres(context.Background(), cfg.Database)
	if err != nil {
		log.Fatal().Err(err).Msg("failed to connect to database")
	}
	defer db.Close()

	rdb, err := cache.NewRedis(cfg.Redis)
	if err != nil {
		log.Fatal().Err(err).Msg("failed to connect to redis")
	}
	defer rdb.Close()

	r := router.New(log, db, rdb, cfg)

	srv := &http.Server{
		Addr:         fmt.Sprintf(":%d", cfg.Port),
		Handler:      r,
		ReadTimeout:  15 * time.Second,
		WriteTimeout: 15 * time.Second,
		IdleTimeout:  60 * time.Second,
	}

	go func() {
		log.Info().Msgf("server starting on port %d", cfg.Port)
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatal().Err(err).Msg("server failed")
		}
	}()

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit

	log.Info().Msg("shutting down server...")

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	if err := srv.Shutdown(ctx); err != nil {
		log.Fatal().Err(err).Msg("server forced to shutdown")
	}

	log.Info().Msg("server stopped")
}
