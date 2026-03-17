.PHONY: build run test lint migrate-up migrate-down docker-build docker-run

build:
	go build -o bin/server ./cmd/server

run:
	go run ./cmd/server

test:
	go test ./... -v -cover

lint:
	golangci-lint run ./...

migrate-up:
	migrate -path migrations -database "$(DB_URL)" up

migrate-down:
	migrate -path migrations -database "$(DB_URL)" down 1

docker-build:
	docker build -t gopost-backend .

docker-run:
	docker-compose up -d
