CREATE TABLE templates (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name            VARCHAR(255) NOT NULL,
    description     TEXT,
    type            VARCHAR(20) NOT NULL CHECK (type IN ('video', 'image')),
    category_id     UUID REFERENCES categories(id),
    creator_id      UUID REFERENCES users(id),
    status          VARCHAR(20) NOT NULL DEFAULT 'draft' CHECK (status IN ('draft', 'review', 'published', 'archived')),
    storage_key     VARCHAR(512),
    thumbnail_url   VARCHAR(512),
    preview_url     VARCHAR(512),
    width           INT,
    height          INT,
    duration_ms     INT,
    layer_count     INT DEFAULT 0,
    editable_fields JSONB DEFAULT '[]'::jsonb,
    usage_count     INT NOT NULL DEFAULT 0,
    is_premium      BOOLEAN NOT NULL DEFAULT false,
    version         INT NOT NULL DEFAULT 1,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    published_at    TIMESTAMPTZ
);

CREATE INDEX idx_templates_type_status ON templates(type, status);
CREATE INDEX idx_templates_category ON templates(category_id);
CREATE INDEX idx_templates_creator ON templates(creator_id);
CREATE INDEX idx_templates_popular ON templates(usage_count DESC) WHERE status = 'published';
CREATE INDEX idx_templates_newest ON templates(published_at DESC) WHERE status = 'published';
CREATE INDEX idx_templates_search ON templates USING gin(to_tsvector('english', name || ' ' || COALESCE(description, '')));

CREATE TABLE template_tags (
    template_id UUID NOT NULL REFERENCES templates(id) ON DELETE CASCADE,
    tag_id      UUID NOT NULL REFERENCES tags(id) ON DELETE CASCADE,
    PRIMARY KEY (template_id, tag_id)
);

CREATE INDEX idx_template_tags_tag ON template_tags(tag_id);
