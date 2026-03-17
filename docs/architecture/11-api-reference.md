## 11. API Reference

### 11.1 Authentication Endpoints

#### POST /api/v1/auth/register

Register a new user account.

**Request:**
```json
{
  "email": "user@example.com",
  "password": "SecureP@ssw0rd!",
  "name": "John Doe",
  "device_fingerprint": "abc123..."
}
```

**Response (201):**
```json
{
  "success": true,
  "data": {
    "user": {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "email": "user@example.com",
      "name": "John Doe",
      "role": "user",
      "created_at": "2026-02-23T10:00:00Z"
    },
    "access_token": "eyJhbGciOiJIUzI1NiIs...",
    "refresh_token": "dGhpcyBpcyBhIHJlZnJl...",
    "expires_in": 900
  }
}
```

**Errors:** `400 VALIDATION_ERROR`, `409 EMAIL_EXISTS`

---

#### POST /api/v1/auth/login

Authenticate and receive tokens.

**Request:**
```json
{
  "email": "user@example.com",
  "password": "SecureP@ssw0rd!",
  "device_fingerprint": "abc123..."
}
```

**Response (200):**
```json
{
  "success": true,
  "data": {
    "user": {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "email": "user@example.com",
      "name": "John Doe",
      "role": "user"
    },
    "access_token": "eyJhbGciOiJIUzI1NiIs...",
    "refresh_token": "dGhpcyBpcyBhIHJlZnJl...",
    "expires_in": 900
  }
}
```

**Errors:** `400 VALIDATION_ERROR`, `401 INVALID_CREDENTIALS`

---

#### POST /api/v1/auth/refresh

Refresh an expired access token.

**Request:**
```json
{
  "refresh_token": "dGhpcyBpcyBhIHJlZnJl..."
}
```

**Response (200):**
```json
{
  "success": true,
  "data": {
    "access_token": "eyJhbGciOiJIUzI1NiIs...",
    "refresh_token": "bmV3IHJlZnJlc2ggdG9r...",
    "expires_in": 900
  }
}
```

**Errors:** `401 INVALID_REFRESH_TOKEN`, `401 REFRESH_TOKEN_EXPIRED`

---

#### POST /api/v1/auth/logout

Invalidate current session.

**Headers:** `Authorization: Bearer <access_token>`

**Response (200):**
```json
{
  "success": true,
  "data": {
    "message": "Logged out successfully"
  }
}
```

---

#### POST /api/v1/auth/oauth/{provider}

OAuth2 authentication (Google, Apple, Facebook).

**Request:**
```json
{
  "id_token": "oauth_provider_id_token...",
  "device_fingerprint": "abc123..."
}
```

**Response:** Same as `/auth/login`.

---

### 11.2 User Endpoints

| Method | Path | Auth | Role | Description |
|--------|------|------|------|-------------|
| GET | `/api/v1/users/me` | Yes | Any | Get current user profile |
| PUT | `/api/v1/users/me` | Yes | Any | Update current user profile |
| DELETE | `/api/v1/users/me` | Yes | Any | Delete account (GDPR) |
| GET | `/api/v1/users/me/data-export` | Yes | Any | Export all user data (GDPR) |
| PUT | `/api/v1/users/me/password` | Yes | Any | Change password |
| GET | `/api/v1/users/me/subscriptions` | Yes | Any | Get subscription status |

---

### 11.3 Template Endpoints

#### GET /api/v1/templates

Browse templates with filtering and pagination.

**Query Parameters:**

| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | string | — | Filter by `video` or `image` |
| `category_id` | UUID | — | Filter by category |
| `tags` | string (comma-sep) | — | Filter by tags |
| `q` | string | — | Full-text search query |
| `sort` | string | `popular` | `popular`, `newest`, `trending` |
| `cursor` | string | — | Cursor for pagination |
| `limit` | int | 20 | Items per page (max 50) |

**Response (200):**
```json
{
  "success": true,
  "data": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "Neon Story Pack",
      "type": "video",
      "thumbnail_url": "https://cdn.gopost.app/thumbs/550e8400.webp",
      "preview_url": "https://cdn.gopost.app/previews/550e8400.mp4",
      "category": { "id": "...", "name": "Stories" },
      "tags": ["neon", "dark", "modern"],
      "usage_count": 15420,
      "is_premium": true,
      "duration_ms": 15000,
      "dimensions": { "width": 1080, "height": 1920 },
      "created_at": "2026-02-20T12:00:00Z"
    }
  ],
  "meta": {
    "next_cursor": "eyJpZCI6...",
    "has_more": true,
    "total": 1523
  }
}
```

---

#### GET /api/v1/templates/:id

Get template details.

**Response (200):**
```json
{
  "success": true,
  "data": {
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "name": "Neon Story Pack",
    "description": "A vibrant neon-themed story template pack with 5 customizable slides.",
    "type": "video",
    "thumbnail_url": "https://cdn.gopost.app/thumbs/550e8400.webp",
    "preview_url": "https://cdn.gopost.app/previews/550e8400.mp4",
    "category": { "id": "...", "name": "Stories" },
    "tags": ["neon", "dark", "modern"],
    "usage_count": 15420,
    "is_premium": true,
    "duration_ms": 15000,
    "dimensions": { "width": 1080, "height": 1920 },
    "layer_count": 8,
    "editable_fields": [
      { "field_id": "title_text", "type": "text", "label": "Title", "default": "Your Story" },
      { "field_id": "bg_image", "type": "image", "label": "Background" },
      { "field_id": "accent_color", "type": "color", "label": "Accent Color", "default": "#FF00FF" }
    ],
    "creator": { "id": "...", "name": "GoPost Team" },
    "version": 3,
    "created_at": "2026-02-20T12:00:00Z",
    "updated_at": "2026-02-22T08:30:00Z"
  }
}
```

---

#### POST /api/v1/templates/:id/access

Request access to download and render a template. Returns a time-limited signed CDN URL and a session decryption key.

**Headers:** `Authorization: Bearer <access_token>`

**Request:**
```json
{
  "device_fingerprint": "abc123...",
  "device_public_key": "MIIBIjANBgkqhki..."
}
```

**Response (200):**
```json
{
  "success": true,
  "data": {
    "download_url": "https://cdn.gopost.app/templates/550e8400.gpt?X-Amz-Signature=...",
    "download_url_expires_at": "2026-02-23T10:10:00Z",
    "session_key_encrypted": "base64_rsa_encrypted_aes_key...",
    "render_token": "eyJhbGciOiJIUzI1NiIs...",
    "render_token_expires_at": "2026-02-23T10:05:00Z"
  }
}
```

**Errors:** `401 UNAUTHORIZED`, `403 SUBSCRIPTION_REQUIRED`, `404 NOT_FOUND`, `429 RATE_LIMITED`

---

#### POST /api/v1/templates (Creator/Admin only)

Publish a new template created via the in-app editor.

**Headers:** `Authorization: Bearer <access_token>`

**Request:** `multipart/form-data`

| Field | Type | Description |
|-------|------|-------------|
| `metadata` | JSON | Name, description, category, tags, type |
| `template_package` | File | Exported .gpt file from in-app editor |
| `thumbnail` | File | Preview thumbnail (WebP, 400x400) |
| `preview` | File | Preview video/image |

**Response (201):**
```json
{
  "success": true,
  "data": {
    "id": "...",
    "status": "pending_review",
    "message": "Template submitted for review"
  }
}
```

---

### 11.4 Admin Endpoints

| Method | Path | Auth | Role | Description |
|--------|------|------|------|-------------|
| GET | `/api/v1/admin/dashboard` | Yes | Admin | Dashboard statistics |
| GET | `/api/v1/admin/templates` | Yes | Admin | List all templates (including pending) |
| PUT | `/api/v1/admin/templates/:id/publish` | Yes | Admin | Publish a pending template |
| PUT | `/api/v1/admin/templates/:id/reject` | Yes | Admin | Reject a pending template |
| DELETE | `/api/v1/admin/templates/:id` | Yes | Admin | Delete a template |
| POST | `/api/v1/admin/templates/upload` | Yes | Admin | Upload external template (AE/Lightroom export) |
| GET | `/api/v1/admin/users` | Yes | Admin | List users with filters |
| PUT | `/api/v1/admin/users/:id/role` | Yes | Admin | Change user role |
| PUT | `/api/v1/admin/users/:id/ban` | Yes | Admin | Ban/unban a user |
| GET | `/api/v1/admin/audit-logs` | Yes | Admin | View audit trail |

---

### 11.5 Subscription Endpoints

| Method | Path | Auth | Role | Description |
|--------|------|------|------|-------------|
| GET | `/api/v1/subscriptions/plans` | No | — | List available subscription plans |
| POST | `/api/v1/subscriptions/checkout` | Yes | Any | Create checkout session (Stripe) |
| POST | `/api/v1/subscriptions/webhook` | No | — | Stripe/RevenueCat webhook |
| POST | `/api/v1/subscriptions/verify` | Yes | Any | Verify purchase (App Store/Play Store receipt) |
| DELETE | `/api/v1/subscriptions/cancel` | Yes | Any | Cancel subscription |

---

### 11.6 Media Endpoints

| Method | Path | Auth | Role | Description |
|--------|------|------|------|-------------|
| POST | `/api/v1/media/upload` | Yes | Any | Upload user media (for editor usage) |
| GET | `/api/v1/media/:id` | Yes | Owner | Get media metadata |
| DELETE | `/api/v1/media/:id` | Yes | Owner | Delete user media |
| POST | `/api/v1/media/:id/export` | Yes | Owner | Export edited result |

---

### 11.7 Category and Tag Endpoints

| Method | Path | Auth | Role | Description |
|--------|------|------|------|-------------|
| GET | `/api/v1/categories` | No | — | List all categories |
| GET | `/api/v1/categories/:id/templates` | No | — | Templates in category |
| GET | `/api/v1/tags/popular` | No | — | Popular tags |
| GET | `/api/v1/tags/search?q=` | No | — | Search tags |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1-5: Foundation through Admin Portal |
| **Sprint(s)** | Sprint 2 (auth), Sprints 3-4 (template), Sprints 12-13 (admin/subscription) |
| **Team** | Go Backend Developer, Flutter Developer |
| **Predecessor** | [04-backend-architecture.md](04-backend-architecture.md) |
| **Successor** | [12-database-schema.md](12-database-schema.md) |
| **Story Points Total** | 88 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| APP-124 | As a new user, I want POST /auth/register so that I can create an account with email and password | - Accepts email, password, name, device_fingerprint<br/>- Returns 201 with user, access_token, refresh_token, expires_in<br/>- Returns 400 VALIDATION_ERROR, 409 EMAIL_EXISTS on failure | 3 | P0 | APP-149 |
| APP-125 | As a user, I want POST /auth/login so that I can authenticate and receive tokens | - Accepts email, password, device_fingerprint<br/>- Returns 200 with user, tokens<br/>- Returns 401 INVALID_CREDENTIALS on failure | 2 | P0 | APP-149 |
| APP-126 | As a user, I want POST /auth/refresh so that I can renew my access token when it expires | - Accepts refresh_token in body<br/>- Returns new access_token and refresh_token (rotation)<br/>- Returns 401 for invalid/expired refresh token | 2 | P0 | APP-125 |
| APP-127 | As a user, I want POST /auth/logout so that I can invalidate my current session | - Requires Bearer token in Authorization header<br/>- Invalidates session in Redis/DB<br/>- Returns 200 with success message | 2 | P0 | APP-125 |
| APP-128 | As a user, I want POST /auth/oauth/{provider} so that I can sign in with Google, Apple, or Facebook | - Accepts id_token and device_fingerprint<br/>- Creates or links user, returns same response as login<br/>- Supports google, apple, facebook providers | 5 | P0 | APP-149 |
| APP-129 | As a user, I want GET/PUT/DELETE /users/me so that I can view, update, and delete my profile | - GET returns current user profile<br/>- PUT updates name, avatar; validates input<br/>- DELETE soft-deletes account (GDPR) | 3 | P0 | APP-124 |
| APP-130 | As a user, I want GET /users/me/data-export so that I can download all my data (GDPR) | - Returns JSON/zip with user data, templates, media<br/>- Requires authentication<br/>- Async job or streaming for large exports | 3 | P0 | APP-129 |
| APP-131 | As a user, I want GET /templates with pagination so that I can browse templates with filters | - Query params: type, category_id, tags, q, sort, cursor, limit<br/>- Returns paginated list with meta.next_cursor, has_more, total<br/>- Integrates with Elasticsearch for full-text search | 5 | P0 | APP-152, APP-156 |
| APP-132 | As a user, I want GET /templates/:id so that I can view template details before downloading | - Returns full template metadata including editable_fields<br/>- Returns 404 for non-existent or unpublished<br/>- Includes creator, version, dimensions, duration | 3 | P0 | APP-152 |
| APP-133 | As a user, I want POST /templates/:id/access so that I can get a signed URL and session key to download and render | - Accepts device_fingerprint, device_public_key<br/>- Returns download_url (signed, time-limited), session_key_encrypted, render_token<br/>- Checks subscription for premium templates; returns 403 if required | 5 | P0 | APP-132 |
| APP-134 | As a creator, I want POST /templates so that I can publish templates from the in-app editor | - Multipart: metadata (JSON), template_package (.gpt), thumbnail, preview<br/>- Validates format, encrypts, stores in S3<br/>- Returns 201 with id, status pending_review | 5 | P0 | APP-152 |
| APP-135 | As an admin, I want GET /admin/dashboard so that I can view key statistics | - Returns user count, template count, recent uploads, pending reviews<br/>- Requires admin role<br/>- Cached for performance | 3 | P0 | APP-129 |
| APP-136 | As an admin, I want GET/PUT/DELETE /admin/templates so that I can manage template lifecycle | - GET lists all templates including pending with filters<br/>- PUT publish/reject pending templates<br/>- DELETE removes template and assets | 5 | P0 | APP-135 |
| APP-137 | As an admin, I want POST /admin/templates/upload so that I can upload external AE/Lightroom exports | - Validates manifest, composition, assets structure<br/>- Converts to .gpt format, encrypts, stores<br/>- Returns template id and status | 5 | P0 | APP-136 |
| APP-138 | As an admin, I want GET/PUT /admin/users so that I can manage user roles and bans | - GET lists users with filters (role, status)<br/>- PUT updates role or ban status<br/>- Audit log entry for each action | 3 | P0 | APP-135 |
| APP-139 | As a user, I want GET /subscriptions/plans so that I can view available subscription tiers | - Returns plans with id, name, price, features<br/>- No auth required<br/>- Integrates with Stripe/RevenueCat product catalog | 2 | P0 | — |
| APP-140 | As a user, I want POST /subscriptions/checkout so that I can start a Stripe checkout session | - Creates Stripe checkout session for selected plan<br/>- Returns session URL or client_secret<br/>- Links to user on success | 3 | P0 | APP-139 |
| APP-141 | As a system, I want POST /subscriptions/webhook so that Stripe/RevenueCat events update subscription status | - Verifies webhook signature<br/>- Handles subscription.created, updated, deleted, payment events<br/>- Idempotent; updates subscriptions and payments tables | 5 | P0 | APP-140 |
| APP-142 | As a user, I want POST /subscriptions/verify so that I can verify App Store/Play Store purchases | - Accepts receipt data from mobile clients<br/>- Validates with Apple/Google APIs<br/>- Updates subscription status, returns current plan | 5 | P0 | APP-139 |
| APP-143 | As a user, I want POST /media/upload so that I can upload media for use in the editor | - Multipart upload to S3/MinIO<br/>- Validates file type, size; virus scan if configured<br/>- Returns media id, storage_key, dimensions/duration | 3 | P0 | — |
| APP-144 | As a user, I want GET/DELETE /media/:id so that I can view metadata and delete my media | - GET returns metadata (owner-only)<br/>- DELETE removes from storage and DB (owner-only)<br/>- Returns 404 for non-existent | 2 | P0 | APP-143 |
| APP-145 | As a user, I want GET /categories so that I can browse templates by category | - Returns all active categories with id, name, slug, icon<br/>- GET /categories/:id/templates returns templates in category<br/>- No auth required | 2 | P0 | APP-156 |
| APP-146 | As a user, I want GET /tags endpoints so that I can filter and search tags | - GET /tags/popular returns most-used tags<br/>- GET /tags/search?q= returns matching tags<br/>- Used for template filtering | 2 | P0 | APP-157 |
| APP-147 | As a developer, I want OpenAPI/Swagger documentation so that API is discoverable and testable | - OpenAPI 3.0 spec generated from code or maintained<br/>- Swagger UI served at /api/docs<br/>- All endpoints documented with request/response schemas | 5 | P1 | APP-124 |
| APP-148 | As a user, I want PUT /users/me/password and GET /users/me/subscriptions so that I can manage account security and view subscription status | - PUT /users/me/password accepts current_password, new_password; validates and updates<br/>- GET /users/me/subscriptions returns current plan, status, period dates<br/>- Both require authentication | 3 | P0 | APP-129, APP-158 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Integration tests passing
- [ ] Documentation updated
- [ ] No critical or high-severity bugs open
- [ ] Sprint review demo completed

---
