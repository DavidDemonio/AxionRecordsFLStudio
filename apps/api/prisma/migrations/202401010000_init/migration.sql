-- Prisma Migration
-- Generated for initial schema

-- Enable pgcrypto so gen_random_uuid() is available on fresh databases
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- Example table that relies on gen_random_uuid(); in the original project this
-- would correspond to the actual schema definitions.
CREATE TABLE IF NOT EXISTS "Example" (
    "id" UUID NOT NULL DEFAULT gen_random_uuid(),
    "name" TEXT NOT NULL,
    "created_at" TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    CONSTRAINT "Example_pkey" PRIMARY KEY ("id")
);
