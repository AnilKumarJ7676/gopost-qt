CREATE TABLE categories (
    id          UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name        VARCHAR(100) NOT NULL UNIQUE,
    slug        VARCHAR(100) NOT NULL UNIQUE,
    description VARCHAR(255),
    icon_url    VARCHAR(512),
    sort_order  INT NOT NULL DEFAULT 0,
    is_active   BOOLEAN NOT NULL DEFAULT true
);

INSERT INTO categories (name, slug, description, sort_order) VALUES
    ('Stories', 'stories', 'Instagram and social media stories', 1),
    ('Reels', 'reels', 'Short-form vertical videos', 2),
    ('Posts', 'posts', 'Social media feed posts', 3),
    ('Thumbnails', 'thumbnails', 'YouTube and video thumbnails', 4),
    ('Presentations', 'presentations', 'Slides and presentations', 5),
    ('Invitations', 'invitations', 'Event and party invitations', 6),
    ('Logos', 'logos', 'Brand logos and marks', 7),
    ('Banners', 'banners', 'Web and social banners', 8),
    ('Flyers', 'flyers', 'Promotional flyers', 9),
    ('Collages', 'collages', 'Photo collages', 10);
