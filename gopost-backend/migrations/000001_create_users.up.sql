CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE users (
    id                 UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    email              VARCHAR(255) NOT NULL,
    password_hash      VARCHAR(255),
    name               VARCHAR(255) NOT NULL,
    avatar_url         VARCHAR(512),
    oauth_provider     VARCHAR(50),
    oauth_id           VARCHAR(255),
    device_fingerprint VARCHAR(255),
    created_at         TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at         TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    deleted_at         TIMESTAMPTZ
);

CREATE UNIQUE INDEX idx_users_email ON users(email) WHERE deleted_at IS NULL;
CREATE INDEX idx_users_oauth ON users(oauth_provider, oauth_id) WHERE oauth_provider IS NOT NULL;
