BEGIN;
CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    email VARCHAR(254) NOT NULL UNIQUE,
    telegram_chat_id VARCHAR(100) NOT NULL DEFAULT '',
    notifications_enabled BOOLEAN NOT NULL DEFAULT TRUE
);
CREATE TABLE IF NOT EXISTS devices (
    id BIGSERIAL PRIMARY KEY,
    sensor_id VARCHAR(128) NOT NULL UNIQUE,
    name VARCHAR(200) NOT NULL,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    enabled BOOLEAN NOT NULL DEFAULT TRUE
);
CREATE INDEX IF NOT EXISTS devices_user_id_idx ON devices(user_id);
-- Existing measurements are retained; device deletion does not erase history.
CREATE TABLE IF NOT EXISTS sensor_data (
    id BIGSERIAL PRIMARY KEY,
    sensor_id VARCHAR(128) NOT NULL,
    value DOUBLE PRECISION NOT NULL,
    unit VARCHAR(64) NOT NULL DEFAULT 'unknown',
    timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    received_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
ALTER TABLE sensor_data ADD COLUMN IF NOT EXISTS received_at TIMESTAMPTZ NOT NULL DEFAULT NOW();
CREATE INDEX IF NOT EXISTS sensor_data_timestamp_idx ON sensor_data(timestamp DESC,id DESC);
-- Drogon 1.9.3 exposes PostgreSQL errors as untyped Failure exceptions.
-- Normalize expected constraint conflicts inside PostgreSQL, without parsing
-- localized error messages. Each function call is one atomic statement.
CREATE OR REPLACE FUNCTION iot_write_user(
    operation TEXT, resource_id BIGINT, user_name TEXT, user_email TEXT,
    chat_id TEXT, notifications BOOLEAN
) RETURNS TABLE(status INTEGER, body JSONB) LANGUAGE plpgsql AS $$
DECLARE saved users%ROWTYPE;
BEGIN
    IF operation = 'create' THEN
        INSERT INTO users(name,email,telegram_chat_id,notifications_enabled)
        VALUES(user_name,user_email,chat_id,notifications) RETURNING * INTO saved;
    ELSIF operation = 'update' THEN
        UPDATE users SET name=user_name,email=user_email,telegram_chat_id=chat_id,
            notifications_enabled=notifications WHERE id=resource_id RETURNING * INTO saved;
    ELSIF operation = 'delete' THEN
        DELETE FROM users WHERE id=resource_id RETURNING * INTO saved;
    ELSE
        RAISE EXCEPTION 'Unsupported repository operation';
    END IF;
    IF saved.id IS NULL THEN
        RETURN QUERY SELECT 404, jsonb_build_object('error','Resource not found');
    ELSIF operation = 'delete' THEN
        RETURN QUERY SELECT 204, 'null'::jsonb;
    ELSE
        RETURN QUERY SELECT CASE WHEN operation = 'create' THEN 201 ELSE 200 END, to_jsonb(saved);
    END IF;
EXCEPTION WHEN unique_violation OR foreign_key_violation THEN
    RETURN QUERY SELECT 409, jsonb_build_object('error','Duplicate resource or conflicting user/device reference');
END;
$$;
CREATE OR REPLACE FUNCTION iot_write_device(
    operation TEXT, resource_id BIGINT, sensor TEXT, device_name TEXT,
    owner_id BIGINT, active BOOLEAN
) RETURNS TABLE(status INTEGER, body JSONB) LANGUAGE plpgsql AS $$
DECLARE saved devices%ROWTYPE;
BEGIN
    IF operation = 'create' THEN
        INSERT INTO devices(sensor_id,name,user_id,enabled)
        VALUES(sensor,device_name,owner_id,active) RETURNING * INTO saved;
    ELSIF operation = 'update' THEN
        UPDATE devices SET name=device_name,user_id=owner_id,enabled=active
            WHERE id=resource_id RETURNING * INTO saved;
    ELSIF operation = 'delete' THEN
        DELETE FROM devices WHERE id=resource_id RETURNING * INTO saved;
    ELSE
        RAISE EXCEPTION 'Unsupported repository operation';
    END IF;
    IF saved.id IS NULL THEN
        RETURN QUERY SELECT 404, jsonb_build_object('error','Resource not found');
    ELSIF operation = 'delete' THEN
        RETURN QUERY SELECT 204, 'null'::jsonb;
    ELSE
        RETURN QUERY SELECT CASE WHEN operation = 'create' THEN 201 ELSE 200 END, to_jsonb(saved);
    END IF;
EXCEPTION WHEN unique_violation OR foreign_key_violation THEN
    RETURN QUERY SELECT 409, jsonb_build_object('error','Duplicate resource or conflicting user/device reference');
END;
$$;
COMMIT;
