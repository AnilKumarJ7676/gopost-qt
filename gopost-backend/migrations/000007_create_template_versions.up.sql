CREATE TABLE template_versions (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    template_id     UUID NOT NULL REFERENCES templates(id) ON DELETE CASCADE,
    version_number  INT NOT NULL,
    storage_key     VARCHAR(512) NOT NULL,
    changelog       VARCHAR(1000),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE(template_id, version_number)
);

CREATE TABLE template_assets (
    id           UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    template_id  UUID NOT NULL REFERENCES templates(id) ON DELETE CASCADE,
    asset_type   VARCHAR(50) NOT NULL,
    storage_key  VARCHAR(512) NOT NULL,
    content_hash VARCHAR(64),
    file_size    BIGINT NOT NULL DEFAULT 0,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_template_assets_template ON template_assets(template_id);
