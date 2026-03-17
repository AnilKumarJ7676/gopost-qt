package config

import (
	"fmt"
	"strings"

	"github.com/spf13/viper"
)

type Config struct {
	Port             int              `mapstructure:"PORT"`
	Env              string           `mapstructure:"ENV"`
	LogLevel         string           `mapstructure:"LOG_LEVEL"`
	CORSOrigins      string           `mapstructure:"CORS_ORIGINS"`
	MigrationsPath   string           `mapstructure:"MIGRATIONS_PATH"`
	ElasticsearchURL string           `mapstructure:"ELASTICSEARCH_URL"`
	Database         DatabaseConfig   `mapstructure:",squash"`
	Redis            RedisConfig      `mapstructure:",squash"`
	JWT              JWTConfig        `mapstructure:",squash"`
	R2               R2Config         `mapstructure:",squash"`
	Cloudflare       CloudflareConfig `mapstructure:",squash"`
}

type R2Config struct {
	AccountID string `mapstructure:"R2_ACCOUNT_ID"`
	Bucket    string `mapstructure:"R2_BUCKET"`
	AccessKey string `mapstructure:"R2_ACCESS_KEY"`
	SecretKey string `mapstructure:"R2_SECRET_KEY"`
	PublicURL string `mapstructure:"R2_PUBLIC_URL"`
}

type CloudflareConfig struct {
	ZoneID     string `mapstructure:"CF_ZONE_ID"`
	APIToken   string `mapstructure:"CF_API_TOKEN"`
	CDNBaseURL string `mapstructure:"CF_CDN_BASE_URL"`
}

type DatabaseConfig struct {
	Host     string `mapstructure:"DB_HOST"`
	Port     int    `mapstructure:"DB_PORT"`
	User     string `mapstructure:"DB_USER"`
	Password string `mapstructure:"DB_PASSWORD"`
	Name     string `mapstructure:"DB_NAME"`
	SSLMode  string `mapstructure:"DB_SSL_MODE"`
	MinConns int32  `mapstructure:"DB_MIN_CONNS"`
	MaxConns int32  `mapstructure:"DB_MAX_CONNS"`
}

func (d DatabaseConfig) DSN() string {
	return fmt.Sprintf(
		"postgres://%s:%s@%s:%d/%s?sslmode=%s",
		d.User, d.Password, d.Host, d.Port, d.Name, d.SSLMode,
	)
}

type RedisConfig struct {
	Host     string `mapstructure:"REDIS_HOST"`
	Port     int    `mapstructure:"REDIS_PORT"`
	Password string `mapstructure:"REDIS_PASSWORD"`
	DB       int    `mapstructure:"REDIS_DB"`
}

func (r RedisConfig) Addr() string {
	return fmt.Sprintf("%s:%d", r.Host, r.Port)
}

type JWTConfig struct {
	Secret          string `mapstructure:"JWT_SECRET"`
	AccessTokenTTL  int    `mapstructure:"JWT_ACCESS_TTL_MINUTES"`
	RefreshTokenTTL int    `mapstructure:"JWT_REFRESH_TTL_DAYS"`
}

func Load() (*Config, error) {
	viper.SetConfigFile(".env")
	viper.AutomaticEnv()
	viper.SetEnvKeyReplacer(strings.NewReplacer(".", "_"))

	viper.SetDefault("PORT", 8080)
	viper.SetDefault("ENV", "development")
	viper.SetDefault("LOG_LEVEL", "debug")
	viper.SetDefault("DB_HOST", "localhost")
	viper.SetDefault("DB_PORT", 5432)
	viper.SetDefault("DB_USER", "gopost")
	viper.SetDefault("DB_PASSWORD", "dev_password")
	viper.SetDefault("DB_NAME", "gopost")
	viper.SetDefault("DB_SSL_MODE", "disable")
	viper.SetDefault("DB_MIN_CONNS", 10)
	viper.SetDefault("DB_MAX_CONNS", 100)
	viper.SetDefault("REDIS_HOST", "localhost")
	viper.SetDefault("REDIS_PORT", 6379)
	viper.SetDefault("REDIS_PASSWORD", "")
	viper.SetDefault("REDIS_DB", 0)
	viper.SetDefault("JWT_SECRET", "dev-secret-change-in-production")
	viper.SetDefault("JWT_ACCESS_TTL_MINUTES", 15)
	viper.SetDefault("JWT_REFRESH_TTL_DAYS", 7)
	viper.SetDefault("R2_ACCOUNT_ID", "")
	viper.SetDefault("R2_BUCKET", "gopost-templates")
	viper.SetDefault("R2_ACCESS_KEY", "")
	viper.SetDefault("R2_SECRET_KEY", "")
	viper.SetDefault("R2_PUBLIC_URL", "")
	viper.SetDefault("CF_ZONE_ID", "")
	viper.SetDefault("CF_API_TOKEN", "")
	viper.SetDefault("CF_CDN_BASE_URL", "https://cdn.gopost.app")
	viper.SetDefault("CORS_ORIGINS", "")
	viper.SetDefault("MIGRATIONS_PATH", "migrations")
	viper.SetDefault("ELASTICSEARCH_URL", "")

	if err := viper.ReadInConfig(); err != nil {
		if _, ok := err.(viper.ConfigFileNotFoundError); !ok {
			return nil, fmt.Errorf("reading config: %w", err)
		}
	}

	var cfg Config
	if err := viper.Unmarshal(&cfg); err != nil {
		return nil, fmt.Errorf("unmarshaling config: %w", err)
	}

	return &cfg, nil
}
